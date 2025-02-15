/***************************************************************************
 * \file WbddcComponent.h
 * \brief Defines the WBDDC interface for the NDR551.
 * \author DA
 * \author NH
 * \author MN
 * \copyright (c) 2017 CyberRadio Solutions, Inc.  All rights reserved.
 *
 ***************************************************************************/

#ifndef INCLUDED_LIBCYBERRADIO_DRIVER_NDR551_WBDDCCOMPONENT_H
#define INCLUDED_LIBCYBERRADIO_DRIVER_NDR551_WBDDCCOMPONENT_H

#include "LibCyberRadio/Driver/WbddcComponent.h"
#include "LibCyberRadio/Common/BasicDict.h"
#include <string>


/**
 * \brief Provides programming elements for controlling CyberRadio Solutions products.
 */
namespace LibCyberRadio
{
    /**
     * \brief Provides programming elements for driving CRS NDR-class radios.
     */
    namespace Driver
    {
        // Forward declaration for RadioHandler
        class RadioHandler;

        /**
         * \brief Provides programming elements for driving NDR551 radios.
         */
        namespace NDR551
        {

            /**
             * \brief WBDDC component class for the NDR551.
             *
             * Configuration dictionary elements:
             * * "enable": Whether or not this DDC is enabled [Boolean/integer/string]
             * * "rateIndex": Rate index [integer/string]
             * * "udpDestination": UDP destination [integer/string]
             * * "vitaEnable": VITA 49 framing setting [integer/string]
             * * "streamId": VITA 49 streamId [integer/string]
             * * "dataPort": Data port index [integer/string]
             *
             */
            class WbddcComponent : public ::LibCyberRadio::Driver::WbddcComponent
            {
                public:
                    /**
                     * \brief Constructs a WbddcComponent object.
                     * \param index The index number of this component.
                     * \param parent A pointer to the RadioHandler object that "owns" this
                     *    component.
                     * \param debug Whether the component supports debug output.
                     * \param dataPort Data port used by this WBDDC.
                     * \param rateIndex WBDDC rate index.  This should be one of the rate
                     *    indices in the WBDDC's rate set.
                     * \param udpDestination UDP destination index.
                     * \param vitaEnable VITA 49 framing setting (0-3).
                     * \param streamId VITA 49 stream ID.
                     */
                    WbddcComponent(int index = 1,
                            ::LibCyberRadio::Driver::RadioHandler* parent = NULL,
                             bool debug = false,
                             int dataPort = 1,
                             int rateIndex = 0,
                             int udpDestination = 0,
                             int vitaEnable = 0,
                             int streamId = 0);
                    /**
                     * \brief Destroys a WbddcComponent object.
                     */
                    virtual ~WbddcComponent();
                    /**
                     * \brief Copies a WbddcComponent object.
                     * \param other The WbddcComponent object to copy.
                     */
                    WbddcComponent(const WbddcComponent& other);
                    /**
                     * \brief Assignment operator for WbddcComponent objects.
                     * \param other The WbddcComponent object to copy.
                     * \returns A reference to the assigned object.
                     */
                    virtual WbddcComponent& operator=(const WbddcComponent& other);
                    /**
                     * \brief Setup Config Dict
                     */
                    void initConfigurationDict() override;
                    /**
                     * \brief Execute a JSON wbddc Query
                     * \param index DDC index
                     * \param rateIndex pointer to data to set.
                     * \param udpDestination pointer to data to set.
                     * \param enabled pointer to data to set.
                     * \param vitaEnable pointer to data to set.
                     * \param streamId pointer to data to set.
                     * \returns True on success
                     */
                    bool executeWbddcQuery(int index, int& rateIndex,
                        int& udpDestination, bool& enabled, int& vitaEnable,
                        unsigned int& streamId);
                    
                    void queryConfiguration() override;

                    bool executeDataPortCommand(int index, int& dataPort) override;
                    bool executeSourceCommand(int index, int& source) override;
                    bool executeFreqCommand(int index, double& freq) override;
                    bool executeWbddcCommand(int index, int& rateIndex,
                                             int& udpDestination, bool& enabled, int& vitaEnable,
                                             unsigned int& streamId) override;

                private:
                    ::LibCyberRadio::Driver::RadioHandler* m551Parent;


            }; // class WbddcComponent

        } /* namespace NDR551 */

    } // namespace Driver

} // namespace LibCyberRadio


#endif // INCLUDED_LIBCYBERRADIO_DRIVER_NDR551_WBDDCCOMPONENT_H
