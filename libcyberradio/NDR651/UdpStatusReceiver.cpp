/*
 * UdpStatusReceiver.cpp
 *
 *  Created on: Dec 1, 2015
 *      Author: nh
 */
#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "LibCyberRadio/NDR651/PacketTypes.h"
#include "LibCyberRadio/NDR651/UdpStatusReceiver.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <algorithm>    // std::min


namespace LibCyberRadio {

    namespace NDR651 {

        UdpStatusReceiver::UdpStatusReceiver(std::string ifname, unsigned int port, bool debug, bool updatePE) :
            LibCyberRadio::Thread("UdpStatusReceiver", "UdpStatusReceiver"),
            LibCyberRadio::Debuggable(debug, "UdpStatusReceiver"),
            _sockfd(-1),
            _shutdown(false),
            _651freeSpace(0), // 2^26 - 2^18
            _sendLock(false),
            _ifname(ifname),
            _port(port),
            _updatePE(updatePE),
            _freeSpaceMax(MAX_RADIO_BUFFSIZE - RADIO_BUFFER_RESERVE),
            timeoutCount(0)
        {
            // TODO Auto-generated constructor stub
            bzero(&_rxbuff, MAX_RX_SIZE);
            FD_ZERO(&set);
            _makeSocket();
        }

        UdpStatusReceiver::~UdpStatusReceiver() {
            // TODO Auto-generated destructor stub
            this->debug("Interrupting\n");
            if (this->isRunning()) {
                this->interrupt();
            }
            _shutdown = true;
            this->_fcMutex.lock();
            this->_selMutex.lock();
            if (_sockfd>=0) {
                this->debug("Closing socket\n");
                //TODO: Close the socket...
                //  this interferes with the select statement in run(), so care must be taken.
            }
            this->debug("Goodbye!\n");
        }

        bool UdpStatusReceiver::_makeSocket(void) {
            int optval; /* flag value for setsockopt */
            struct sockaddr_in serveraddr; /* server's addr */
            char ip_addr_string[INET_ADDRSTRLEN];
            _selMutex.lock();
            // Kill existing socket if it exists.
            if (_sockfd>=0) {
                close(_sockfd);
                _sockfd = -1;
                FD_ZERO(&set);
            }
            // Create new socket.
            if ((_sockfd<0)&&(_port>0)) {
                _sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (_sockfd<0) {
                    std::cerr << "Error opening socket" << std::endl;
                    return false;
                }

                optval = 1;
                setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR,
                        (const void *)&optval , sizeof(int));

                // We're binding to a specific device. With this, we won't need to bind to an IP.
                //~ setsockopt(_sockfd, SOL_SOCKET, SO_BINDTODEVICE,
                //~ (void *)_ifname.c_str(), _ifname.length()+1);

                /*
                 * build the server's Internet address
                 */
                memset((char *) &serveraddr, 0, sizeof(serveraddr));
                serveraddr.sin_family = AF_INET;
                serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
                inet_ntop(AF_INET, &(serveraddr.sin_addr), ip_addr_string, INET_ADDRSTRLEN);
                serveraddr.sin_port = htons((unsigned short)_port);
                /*
                 * bind: associate the parent socket with a port
                 */
                if (bind(_sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
                    std::cerr << "ERROR on binding socket" << std::endl;
                    _sockfd = -1;
                    return false;
                } else {
                    std::cerr << "Status socket bound" << std::endl;
                }
                FD_ZERO(&set);
                FD_SET(_sockfd, &set);
            }
            _selMutex.unlock();
            return _sockfd>=0;
        }

        bool UdpStatusReceiver::setStatusInterface(std::string ifname) {
            return setStatusInterface(ifname, true);
        }

        bool UdpStatusReceiver::setStatusInterface(std::string ifname, bool makeSocketFlag) {
            _ifname = ifname;
            if (makeSocketFlag) {
                return _makeSocket();
            } else {
                return true;
            }
        }

        bool UdpStatusReceiver::setStatusPort(unsigned int port) {
            _port = port;
            return setStatusPort(port, true);
        }

        bool UdpStatusReceiver::setStatusPort(unsigned int port, bool makeSocketFlag) {
            _port = port;
            if (makeSocketFlag) {
                return _makeSocket();
            } else {
                return true;
            }
        }

        bool UdpStatusReceiver::setUpdatePE(bool updatePE) {
            std::cerr << std::endl << std::endl << "setUpdatePE(" << updatePE << ")" << std::endl << std::endl;
            _fcMutex.lock();
            _selMutex.lock();
            _updatePE = updatePE;
            _fcMutex.unlock();
            _selMutex.unlock();
            return _updatePE==updatePE;
        }

        int UdpStatusReceiver::setMaxFreeSpace(float fs, float maxLatency) {
            int maxSamplesLatency = (int)std::floor(maxLatency*fs);
            int maxSamplesLog2;
            _fcMutex.lock();
            _freeSpaceMax = std::min( MAX_RADIO_BUFFSIZE-RADIO_BUFFER_RESERVE, maxSamplesLatency );
            _fcMutex.unlock();
            return _freeSpaceMax;
        }

        void UdpStatusReceiver::run() {
            std::cout << "UdpStatusReceiver::run() " << _651freeSpace << std::endl;
            struct timespec spec;
            struct timeval tout;
            struct sockaddr_in clientaddr; /* client addr */
            socklen_t clientlen = sizeof(clientaddr); /* byte size of client's address */
            struct TxStatusFrame * status;
            int numBytesRx;
            long int oldFreeSpace = 0;
            while(this->isRunning() && (!_shutdown)) {
                tout.tv_sec = (long int) 0;
                tout.tv_usec = (long int) 500000;
                FD_ZERO(&set);
                FD_SET(_sockfd, &set);
                _selMutex.lock();
                if (select(FD_SETSIZE, &set, NULL, NULL, &tout)>0) {
                    //std::cout << "Select trigger" << std::endl;
                    numBytesRx = recvfrom(_sockfd, _rxbuff, MAX_RX_SIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
                    _selMutex.unlock();
                    //std::cout << "# Bytes Rx = " << numBytesRx << std::endl;
                    if (numBytesRx==sizeof(TxStatusFrame)) {
                        status = (struct TxStatusFrame *)_rxbuff;
                        oldFreeSpace = _651freeSpace;
                        if ((bool)status->status.PE||(bool)status->status.PF) {
                            std::cout << "DUCHS FRAME: PEF = " << status->status.PP << status->status.PE << status->status.PF;
                            std::cout << ", Free Space = " << status->status.spaceAvailable << " samples";
                            std::cout << std::endl;
                        }
                        if ( this->_setFreeSpace( status->status.spaceAvailable, (bool)status->status.PP, (bool)status->status.PE, (bool)status->status.PF ) ) {
                            if (status->status.PE) {
                                std::cout << "Free space = " << oldFreeSpace << "->" << _651freeSpace << "?" << status->status.spaceAvailable << " [" << _freeSpaceMax << "]";
                                std::cout << " (" << status->status.PP << status->status.PE << status->status.PF << "), ";
                                std::cout << "@ time = " << status->v49.timeSeconds << " " << status->v49.timeFracSecMSB << " " << status->v49.timeFracSecLSB << std::endl;
                            }
                        }
                        if (status->status.emptyFlag||status->status.underrunFlag||status->status.overrunFlag||status->status.packetLossFlag) {
                            //~ if (status->status.emptyFlag||status->status.underrunFlag||status->status.overrunFlag) {
                            std::cerr << "<" << status->v49.streamId << "@" << status->v49.timeSeconds << "." << status->v49.timeFracSecLSB << ":";
                            if (status->status.PP) {
                                std::cerr << "P";
                            }
                            if (status->status.PE) {
                                std::cerr << "E";
                            }
                            if (status->status.PF) {
                                std::cerr << "F";
                            }
                            std::cerr << "(" << ( status->status.spaceAvailable-67108862 ) << ")";
                            if (status->status.emptyFlag) {
                                std::cerr << "_e";
                            }
                            if (status->status.underrunFlag) {
                                std::cerr << "_u" << status->status.underrunCount;
                            }
                            if (status->status.overrunFlag) {
                                std::cerr << "_o" << status->status.overrunCount;
                            }
                            if (status->status.packetLossFlag) {
                                std::cerr << "_p" << status->status.packetLossCount;
                            }
                            std::cerr << "> " << std::endl;
                        }
                        //~ else if (status->status.PF) {
                        //~ std::cout << "\tFree space notification = " << status->status.spaceAvailable << ", current = " << _651freeSpace;
                        //~ std::cout << "@ time = " << status->v49.timeSeconds << " " << status->v49.timeFracSecMSB << " " << status->v49.timeFracSecLSB << std::endl;
                        //~ }
                    }
                } else {
                    _selMutex.unlock();
                    timeoutCount += 1;
                    //~ this->debug("Timeout\n");
                    //~ usleep(1000);
                }
                this->sleep(2e-6);
                //~ _selMutex.unlock();
            }
        }

        bool UdpStatusReceiver::_setFreeSpace(int updateFromRadio, bool flagPP, bool flagPE, bool flagPF) {
            bool updated = false;
            _fcMutex.lock();
            if (((!_updatePE)&&flagPP)||(_updatePE&&flagPE)) {
                _651freeSpace = std::min( _freeSpaceMax, updateFromRadio - RADIO_BUFFER_RESERVE );
                updated = true;
            }
            _fcMutex.unlock();
            return updated;
        }

        bool UdpStatusReceiver::okToSend(long int numSamples, bool lockIfOk) {
            _fcMutex.lock();
            bool ok = _651freeSpace>=numSamples;
            if (!(ok&&lockIfOk)) {
                _fcMutex.unlock();
            } else {
                _sendLock = true;
            }
            return ok;
        }

        long int UdpStatusReceiver::getFreeSpace(void) {
            boost::mutex::scoped_lock lock(_fcMutex);
            return _651freeSpace;
        }

        bool UdpStatusReceiver::sentNSamples(long int samplesSent) {
            if (!_sendLock) {
                _fcMutex.lock();
            }
            _651freeSpace -= samplesSent;
            _fcMutex.unlock();
            return _651freeSpace>0;
        }

    } /* namespace NDR651 */

} /* namespace CyberRadio */
