/*******************************************************************************
  Copyright(c) 2016 Magnus W. Eriksen. All rights reserved.

  ServoBlaster Cap, Driver for DIY servo driven dustcaps.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/
#include "servoblaster_cap.h"

#include <memory>
#include <string.h>

#define CALIB_TAB "Calibrate"

#define SERVO_TRAVEL_TIME 5000.00
#define SERVO_POLLMS 50.00
#define SERVO_MULTIPYER 100

// We declare an auto pointer to ServoBlasterCap.
std::unique_ptr<ServoBlasterCap> servoblaster_cap(new ServoBlasterCap());

void ISGetProperties(const char *dev)
{
    servoblaster_cap->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    servoblaster_cap->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    servoblaster_cap->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    servoblaster_cap->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    servoblaster_cap->ISSnoopDevice(root);
}

ServoBlasterCap::ServoBlasterCap() : LightBoxInterface(this, false)
{
    setVersion(0,1);
    // Initialize all vars for predictable behavior.
    isConnecting = true;
    isMoving = false;
    isMoveStep = false;
    isClosing = false;
    isReversed = false;
    steps = 0;
}

ServoBlasterCap::~ServoBlasterCap()
{

}

bool ServoBlasterCap::initProperties()
{
    INDI::DefaultDevice::initProperties();

    /************************************************************************************
    * Main Tab
    * ***********************************************************************************/
    IUFillNumber(&MoveSteppN[0], "STEPP_OPEN", "stepp open (ms)", "%6.2f", 0, 0.25, 0.1, 0);
    IUFillNumber(&MoveSteppN[1], "STEPP_CLOSE", "stepp close (ms)", "%6.2f", 0, 0.25, 0.1, 0);
    IUFillNumberVector(&MoveSteppNP, MoveSteppN, 2, getDeviceName(), "STEPP_MOVE", "Stepp",
            MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&AbsolutePosN[0], "ABS_POS", "abs position (ms)", "%6.2f", 0.4, 2.6, 0.1, 0);
    IUFillNumberVector(&AbsolutePosNP, AbsolutePosN, 1, getDeviceName(), "ABSOLUTE_POSITION",
            "Servo position", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    initDustCapProperties(getDeviceName(), MAIN_CONTROL_TAB);
    initLightBoxProperties(getDeviceName(), MAIN_CONTROL_TAB);

    /************************************************************************************
    * Options Tab
    * ***********************************************************************************/
    IUFillSwitch(&LightTypeS[0], "TYPE_USBRELAY2", "use USBRelay2", ISS_ON);
    IUFillSwitch(&LightTypeS[1], "TYPE_GPIO", "use WiringPi GPIO", ISS_OFF);
    IUFillSwitchVector(&LightTypeSP, LightTypeS, 2, getDeviceName(), "TYPE_SELECT",
            "Light device", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

    /************************************************************************************
    * Calibration Tab
    * ***********************************************************************************/
    IUFillNumber(&ServoIDN[0], "SERVO_ID", "set servo id (0-7)", "%6.2f", 0, 7, 1, 0);
    IUFillNumberVector(&ServoIDNP, ServoIDN, 1, getDeviceName(), "SERVO_ID",
            "Servo ID", CALIB_TAB, IP_RW, 60, IPS_OK);

    IUFillNumber(&LightSwitchN[0], "LIGHT_SWITCH", "set flat light device", "%6.2f", 0, 9, 1, 0);
    IUFillNumberVector(&LightSwitchNP, LightSwitchN, 1, getDeviceName(), "LIGHT_SWITCH",
            "Light switch dev nbr", CALIB_TAB, IP_RW, 60, IPS_OK);

    IUFillSwitch(&ReverseTravelS[0], "REVERSE_DISABLE", "normal direction", ISS_ON);
    IUFillSwitch(&ReverseTravelS[1], "REVERSE_ENABLE", "reversed direction", ISS_OFF);
    IUFillSwitchVector(&ReverseTravelSP,ReverseTravelS, 2, getDeviceName(), "REVERSE_SELECT",
            "Travel direction", CALIB_TAB,IP_RW, ISR_1OFMANY, 0, IPS_OK);

    IUFillNumber(&ServoTravelN[0], "LIMIT_OPEN", "set open travel (ms)", "%6.2f", 1.6, 2.6, 0.1, 1.8);
    IUFillNumber(&ServoTravelN[1], "LIMIT_CLOSE", "set close travel (ms)", "%6.2f", 0.4, 1.4, 0.1, 1.2);
    IUFillNumberVector(&ServoTravelNP, ServoTravelN, 2, getDeviceName(), "ROOF_TRAVEL_LIMITS",
            "Max travel Limits", CALIB_TAB, IP_RW, 60, IPS_OK);

    IUFillNumber(&ServoTravelReverseN[0], "LIMIT_OPEN_REVERSE", "set open travel (ms)", "%6.2f", 0.4, 1.4, 0.1, 1.2);
    IUFillNumber(&ServoTravelReverseN[1], "LIMIT_CLOSE_REVERSE", "set close travel (ms)", "%6.2f", 1.6, 2.6, 0.1, 1.8);
    IUFillNumberVector(&ServoTravelReverseNP, ServoTravelReverseN, 2, getDeviceName(), "ROOF_TRAVEL_LIMITS_REVERSE",
            "Max travel Limits", CALIB_TAB, IP_RW, 60, IPS_OK);

    IUFillNumber(&ServoLimitN[0], "LIMIT_OPEN", "set open limit (%)", "%6.2f", 0, 100, 1, 100);
    IUFillNumber(&ServoLimitN[1], "LIMIT_CLOSE", "set close limit (%)", "%6.2f", 0, 100, 1, 100);
    IUFillNumberVector(&ServoLimitNP, ServoLimitN, 2, getDeviceName(), "ROOF_PREFERED_LIMITS",
            "Prefered Limits", CALIB_TAB, IP_RW, 60, IPS_OK);

    setDriverInterface(AUX_INTERFACE | DUSTCAP_INTERFACE | LIGHTBOX_INTERFACE);
    addDebugControl();
    addSimulationControl();

    return true;
}

void ServoBlasterCap::SetupParams()
{
    // Initialize the Parkdata class
    if (parkData.InitPark())
    {
        IUResetSwitch(&ParkCapSP);
        // Check park status and update ParkCapSP
        if (parkData.isParked())
        {
            DEBUG(INDI::Logger::DBG_SESSION, "Parkstatus initialized to parked");
            ParkCapS[0].s = ISS_ON;
            ParkCapSP.s = IPS_OK;
            IDSetSwitch(&ParkCapSP, NULL);
            isClosing = true;
        } 
        else
        {
            DEBUG(INDI::Logger::DBG_SESSION, "Parkstatus initialized to unparked");
            ParkCapS[1].s = ISS_ON;
            ParkCapSP.s = IPS_OK;
            IDSetSwitch(&ParkCapSP, NULL);
            isClosing = false;
        }
    }
}

void ServoBlasterCap::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
}

bool ServoBlasterCap::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        // Main tab
        defineSwitch(&ParkCapSP);
        defineSwitch(&LightSP);
        defineNumber(&MoveSteppNP);
        defineNumber(&AbsolutePosNP);

        // Options tab
        defineSwitch(&LightTypeSP);

        // Calib tab
        // ServoTravelReverseNP will be defined when we get ReverseChannelSP.
        // See isNewSwitch
        defineNumber(&ServoIDNP);
        defineNumber(&LightSwitchNP);
        defineSwitch(&ReverseTravelSP);
        defineNumber(&ServoTravelNP);
        defineNumber(&ServoLimitNP);
    }
    else
    {
        // Main tab
        deleteProperty(ParkCapSP.name);
        deleteProperty(LightSP.name);
        deleteProperty(MoveSteppNP.name);
        deleteProperty(AbsolutePosNP.name);

        // Options tab
        deleteProperty(LightTypeSP.name);

        // Calib tab
        deleteProperty(ServoIDNP.name);
        deleteProperty(LightSwitchNP.name);
        deleteProperty(ReverseTravelSP.name);
        deleteProperty(ServoTravelNP.name);
        deleteProperty(ServoTravelReverseNP.name);
        deleteProperty(ServoLimitNP.name);
    }

    return true;
}


const char * ServoBlasterCap::getDefaultName()
{
    return (char *)"ServoBlaster Cap";
}

bool ServoBlasterCap::Connect()
{
    // Use pgrep to see if servod (ServoBlaster) service is running.
    // If not, then only set driver online if simulated.
    std::string cmd = "pgrep servod";
    int res = PopenInt(cmd.c_str());
    DEBUGF(INDI::Logger::DBG_DEBUG, "*****Connect res = %d", res);

    if (INDI::DefaultDevice::isSimulation())
        DEBUGF(INDI::Logger::DBG_SESSION, "%s is online (simulated)", getDeviceName());
    else if (res != 0) 
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s is offline, did not initialize. Service servod not running", getDeviceName());
        return false;
    } 
    else
        DEBUGF(INDI::Logger::DBG_SESSION, "%s is online.", getDeviceName());

    SetupParams();
    SetTimer(350);
    return true;
}

bool ServoBlasterCap::Disconnect()
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s is offline.", getDeviceName());

    return true;
}

bool ServoBlasterCap::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        if (processLightBoxNumber(dev, name, values, names, n))
            return true;

        // Move in steps, useful when calibrating limits.
        if (strcmp(name, MoveSteppNP.name)==0)
        {
            if (values[0] != 0 && values[1] != 0)
            {
                DEBUG(INDI::Logger::DBG_SESSION
                        , "Cannot set step open and step close at the same time. Set one field to zero");
                return false;
            } 
            else if (values[0] == 0 && values[1] == 0)
            {
                DEBUG(INDI::Logger::DBG_SESSION
                        , "Both step values are zero, select a value to step and try again");
                return false;
            }
            else {
                IUUpdateNumber(&MoveSteppNP, values, names, n);
                MoveSteppNP.s = IPS_OK;
                IDSetNumber(&MoveSteppNP, NULL);

                isClosing = values[0] == 0 ? true : false;
                MoveStep(isClosing ? values[1] : values[0]);

                return true;
            }
        }
        // Update all remainig switches.
        INumberVectorProperty *updateNP = NULL;
        if (strcmp(name, ServoIDNP.name)==0)
        {
            updateNP = &ServoIDNP;
        } 
        else if (strcmp(name, LightSwitchNP.name)==0)
        {
            updateNP = &LightSwitchNP;
        }
        else if (strcmp(name, ServoTravelNP.name)==0)
        {
            updateNP = &ServoTravelNP;
        }
        else if (strcmp(name, ServoTravelReverseNP.name)==0)
        {
            updateNP = &ServoTravelReverseNP;
        }
        else if (strcmp(name, ServoLimitNP.name)==0)
        {
            updateNP = &ServoLimitNP;
        }

        if (updateNP != NULL)
        {
            IUUpdateNumber(updateNP, values, names, n);
            updateNP->s = IPS_OK;
            IDSetNumber(updateNP, NULL);
            DEBUGF(INDI::Logger::DBG_DEBUG, "*****SetNumber->name == %s", updateNP->name);
            return true;
        }

    }

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool ServoBlasterCap::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        if (processLightBoxText(dev, name, texts, names, n))
            return true;
    }
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool ServoBlasterCap::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        if (processLightBoxSwitch(dev, name, states, names, n))
            return true;

        if (processDustCapSwitch(dev, name, states, names, n))
            return true;

        // Update if we are using USBRelay2, or WiringPi as light source.
        if (strcmp(name, LightTypeSP.name)==0)
        {
            IUUpdateSwitch(&LightTypeSP, states, names, n);
            LightTypeSP.s = IPS_OK;
            IDSetSwitch(&LightTypeSP, NULL);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Light switch type set to %s"
                    , strcmp(names[0], "TYPE_USBRELAY2") == 0 ? "USBRelay2 Roof." : "WiringPi GPIO");
        }
        else if (strcmp(name, ReverseTravelSP.name)==0)
        {
            // Set Normal direction. Also called when receiving ReverseTravelSP on connect.
            if (strcmp(names[0], "REVERSE_DISABLE")==0 && states[0] == ISS_ON) {
                double open = ServoTravelReverseN[0].value;
                double closed = ServoTravelReverseN[1].value;
                deleteProperty(ServoTravelReverseNP.name);
                deleteProperty(ServoLimitNP.name);
                defineNumber(&ServoTravelNP);
                defineNumber(&ServoLimitNP);
                ServoTravelN[0].value = closed;
                ServoTravelN[1].value = open;
                ServoTravelNP.s = IPS_OK;
                IDSetNumber(&ServoTravelNP, NULL);
                DEBUG(INDI::Logger::DBG_SESSION, "Travel limits normal");
            } else {
                // Set reversed direction.
                double open = ServoTravelN[0].value;
                double closed = ServoTravelN[1].value;
                deleteProperty(ServoTravelNP.name);
                deleteProperty(ServoLimitNP.name);
                defineNumber(&ServoTravelReverseNP);
                defineNumber(&ServoLimitNP);
                ServoTravelReverseN[0].value = closed;
                ServoTravelReverseN[1].value = open;
                ServoTravelReverseNP.s = IPS_OK;
                IDSetNumber(&ServoTravelReverseNP, NULL);
                DEBUG(INDI::Logger::DBG_SESSION, "Travel limits reversed");
            }
            IUUpdateSwitch(&ReverseTravelSP, states, names, n);
            ReverseTravelSP.s = IPS_OK;
            IDSetSwitch(&ReverseTravelSP, NULL);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool ServoBlasterCap::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool ServoBlasterCap::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &ServoIDNP);
    IUSaveConfigNumber(fp, &LightSwitchNP);
    IUSaveConfigSwitch(fp, &LightTypeSP);
    IUSaveConfigSwitch(fp, &ReverseTravelSP);
    IUSaveConfigNumber(fp, &ServoTravelNP);
    IUSaveConfigNumber(fp, &ServoTravelReverseNP);
    IUSaveConfigNumber(fp, &ServoLimitNP);
    return true;
}

IPState ServoBlasterCap::ParkCap()
{    
    double absAtStart = AbsolutePosN[0].value;
    isClosing = true;

    IUResetSwitch(&ParkCapSP);
    ParkCapS[0].s = ISS_ON;
    ParkCapSP.s = IPS_BUSY;
    IDSetSwitch(&ParkCapSP, NULL);

    if (!isConnecting)
        return Move(absAtStart) ? IPS_BUSY : IPS_ALERT;
    return IPS_OK;
}

IPState ServoBlasterCap::UnParkCap()
{
    double absAtStart = AbsolutePosN[0].value;
    isClosing = false;

    IUResetSwitch(&ParkCapSP);
    ParkCapS[1].s = ISS_ON;
    ParkCapSP.s = IPS_BUSY;
    IDSetSwitch(&ParkCapSP, NULL);

    if (!isConnecting)
        return Move(absAtStart) ? IPS_BUSY : IPS_ALERT;
    return IPS_OK;
}

void ServoBlasterCap::SetOKParkStatus()
{
    // Update park status after TimerHit has completed.
    IUResetSwitch(&ParkCapSP);
    if (isClosing)
        ParkCapS[0].s = ISS_ON;
    else
        ParkCapS[1].s = ISS_ON;
    ParkCapSP.s = IPS_OK;
    IDSetSwitch(&ParkCapSP, NULL);

    parkData.SetParked(isClosing);
}

bool ServoBlasterCap::Move(double absAtStart)
{
    // Initialize limit, steps and start TimerHit to prak or unpark cap.
    limit = getFullABS(isClosing);
    double toMove = limit - absAtStart;
    steps = toMove * (SERVO_POLLMS / SERVO_TRAVEL_TIME);
    isMoving = true;

    // No need to have external light on when closing cap.
    if (isClosing)
    {
        ISState *states;
        ISState s[2] = {ISS_OFF, ISS_ON};
        states = s;
        char *names[2] = {strdup("FLAT_LIGHT_ON"), strdup("FLAT_LIGHT_OFF")};
        ISNewSwitch(getDeviceName(), LightSP.name, states, names, 2);
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "%s from %6.2f, to %6.2f with steps of %6.4f"
            , isClosing ? "Closing" : "Opening", absAtStart, limit, steps);
    SetTimer(SERVO_POLLMS);

    return true;
}

bool ServoBlasterCap::MoveStep(double toMove)
{
    // initialize limit, steps and start TimerHit to open or close cap in steps.
    // Usefull when calibrating.
    toMove *= isClosing ? -1 : 1;
    toMove *= isReversed ? -1 : 1;
    steps = toMove * (SERVO_POLLMS / (SERVO_TRAVEL_TIME * 0.25));
    limit = AbsolutePosN[0].value + toMove;
    isMoving = true;
    isMoveStep = true;

    DEBUGF(INDI::Logger::DBG_SESSION, "Stepping %s from %6.2f, to %6.2f with steps of %6.4f"
            , isClosing ? "closed" : "open", AbsolutePosN[0].value, limit, steps);
    SetTimer(SERVO_POLLMS);

    return true;
}

bool ServoBlasterCap::EnableLightBox(bool enable)
{   
    int powerSwitch = LightSwitchN[0].value;
    bool usbrelay2 = LightTypeS[0].s == ISS_ON ? true : false;
    
    // indi_setprop returns nothing with popen on fail or success.
    // Use indi_getprop to test that we can read the desired property. ie. it exists.
    std::string check_cmd;
    if (usbrelay2)
        check_cmd = "indi_getprop \"USBRelay2 Roof.POWER_SWITCH_" + std::to_string(powerSwitch) + ".POWER_ON_SWITCH\"";
    else
        check_cmd = "indi_getprop \"WiringPi GPIO.OUTPUT_VECTOR_" + std::to_string(powerSwitch) + ".OUTPUT_ACTIVE_" + std::to_string(powerSwitch) + "\"";
    
    int ret = PopenInt(check_cmd.c_str());

    if(ret != 0)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s flat light failed. Attempted test cmd was: %s", enable ? "Enable" : "Disable", check_cmd.c_str());
        DEBUGF(INDI::Logger::DBG_ERROR, "%s flat light failed. Is %s %d defined?"
                , enable ? "Enable" : "Disable", usbrelay2 ? "USBRelay2 Roof connected, and Power Switch" : "WiringPi GPIO connected, and Pin", powerSwitch);

        return false;
    }

    // Use indi_setprop to turn on or off lightsource. 
    std::string cmd;
    if (enable)
        if (usbrelay2)
            cmd = "indi_setprop \"USBRelay2 Roof.POWER_SWITCH_" + std::to_string(powerSwitch) + ".POWER_ON_SWITCH=On\"";
        else
            cmd = "indi_setprop \"WiringPi GPIO.OUTPUT_VECTOR_" + std::to_string(powerSwitch) + ".OUTPUT_ACTIVE_" + std::to_string(powerSwitch) + "=On\"";
    else
        if (usbrelay2)
            cmd = "indi_setprop \"USBRelay2 Roof.POWER_SWITCH_" + std::to_string(powerSwitch) + ".POWER_OFF_SWITCH=On\"";
        else
            cmd = "indi_setprop \"WiringPi GPIO.OUTPUT_VECTOR_" + std::to_string(powerSwitch) + ".OUTPUT_INACTIVE_" + std::to_string(powerSwitch) + "=On\"";

    PopenInt(cmd.c_str());

    DEBUGF(INDI::Logger::DBG_SESSION, "Light source %s.", enable ? "enabled" : "disabled");
    return true;
}
 
bool ServoBlasterCap::SetLightBoxBrightness(uint16_t value)
{
    return false;
}

void ServoBlasterCap::TimerHit()
{
    if (isConnected() == false)
        return;

    // Set the abs position after we are sure all properties are loaded.
    if (isConnecting)
    {
        setABS(getFullABS(isClosing));
        isConnecting = false;
        return;
    }

    // Move. Used by park / unpark, and move step open / close.
    if (isMoving)
    {
        double servoTo = AbsolutePosN[0].value + steps;

        if ((isClosing && !isReversed) || (!isClosing && isReversed)) {
            if (servoTo <= limit)
                isMoving = false;
            else
                SetTimer(SERVO_POLLMS);
        } else {
            if (servoTo >= limit)
                isMoving = false;
            else
                SetTimer(SERVO_POLLMS);
        }

        if (isMoving)
        {
            // Only move servo and update abs if we are allowed to move
            std::string cmd = "echo " 
                + std::to_string(static_cast<int>(ServoIDN[0].value))
                + "=" 
                + std::to_string(static_cast<int>(servoTo * SERVO_MULTIPYER))
                + "> /dev/servoblaster";
            if (!INDI::DefaultDevice::isSimulation())
            {
                PopenInt(cmd.c_str());
            }
            DEBUGF(INDI::Logger::DBG_DEBUG, "*****Moving servo with CMD: %s", cmd.c_str());

            setABS(servoTo);
        }
        if (!isMoving && !isMoveStep)
            SetOKParkStatus();
        if (!isMoving)
            isMoveStep = false;
    }
}

double ServoBlasterCap::getFullABS(bool closed)
{
    // Get the fully open or fully closed position for our calibration.
    // No chance of reading physical servo position, so abs position is always calculated.
    isReversed = ReverseTravelS[0].s == ISS_ON ? false : true;
    double pos;
    if (!closed)
    {
        if (!isReversed)
            pos = ServoTravelN[0].value * (ServoLimitN[0].value / 100);
        else
            pos = ServoTravelReverseN[0].value * (1 + (100 - ServoLimitN[0].value) / 100);
    } else {
        if (!isReversed)
            pos = ServoTravelN[1].value * (1 + (100 - ServoLimitN[1].value) / 100);
        else
            pos = ServoTravelReverseN[1].value * (ServoLimitN[1].value / 100);
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "*****getFullABS, closed %s, reversed %s, pos %6.2f"
            , closed ? "true" : "false", isReversed ? "true" : "false", pos);
    return pos;
}

void ServoBlasterCap::setABS(double pos)
{
    // Update the abs position when moving or initializing on connect.
    AbsolutePosN[0].value = pos;
    AbsolutePosNP.s = IPS_OK;
    IDSetNumber(&AbsolutePosNP, NULL);
}

int ServoBlasterCap::PopenInt(const char* cmd)
{
    // popen to issue system calls with return.
    FILE *fp;
    char buffer[512];

    if (!(fp = popen(cmd,"r")))
        return 1;
    
    std::string str = "";
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
        str += buffer;
    pclose(fp);

    DEBUGF(INDI::Logger::DBG_DEBUG, "*****PopenString: %s", str.c_str());
    // str == "" then popen did not return anything.
    return str == "" ? 1 : 0;
}
