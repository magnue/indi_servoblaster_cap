/*******************************************************************************
  Parkdata, collected from original code in indidome.cpp

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
#include <parkdata.h>
#include <string.h>

Parkdata::Parkdata()
{
    IsParked = false;
    Parkdatafile = "~/.indi/ParkData.xml";
    XMLEle *ParkdataXmlRoot, *ParkdeviceXml, *ParkstatusXml;
}

Parkdata::~Parkdata()
{

}

char *Parkdata::getDeviceName()
{
    return (char *)"ServoBlaster Cap";
}

void Parkdata::SetParked(bool isparked)
{  
    IsParked=isparked;  

    if (IsParked)
    {      
        DEBUG( INDI::Logger::DBG_SESSION, "Cap is parked.");
    }
    else
    {      
        DEBUG( INDI::Logger::DBG_SESSION, "Cap is unparked.");
    }  

    WriteParkData();
}

bool Parkdata::isParked()
{
    return IsParked;
}

bool Parkdata::InitPark()
{
    char *loadres;
    loadres=LoadParkData();
    if (loadres)
    {
        DEBUGF( INDI::Logger::DBG_SESSION, "InitPark: No Park data in file %s: %s", Parkdatafile, loadres);
        SetParked(false);
        return false;
    }  

    return true;
}

char *Parkdata::LoadParkData()
{
    wordexp_t wexp;
    FILE *fp;
    LilXML *lp;
    static char errmsg[512];

    XMLEle *parkxml;
    XMLAtt *ap;
    bool devicefound=false;

    ParkstatusXml=NULL;
    ParkdeviceXml=NULL;
    ParkDeviceName = getDeviceName();

    if (wordexp(Parkdatafile, &wexp, 0))
    {
        wordfree(&wexp);
        return (char *)("Badly formed filename.");
    }

    if (!(fp=fopen(wexp.we_wordv[0], "r")))
    {
        wordfree(&wexp);
        return strerror(errno);
    }
    wordfree(&wexp);

    lp = newLilXML();

    if (ParkdataXmlRoot)
        delXMLEle(ParkdataXmlRoot);

    ParkdataXmlRoot = readXMLFile(fp, lp, errmsg);

    delLilXML(lp);
    if (!ParkdataXmlRoot)
        return errmsg;

    if (!strcmp(tagXMLEle(nextXMLEle(ParkdataXmlRoot, 1)), "parkdata"))
        return (char *)("Not a park data file");

    parkxml=nextXMLEle(ParkdataXmlRoot, 1);

    while (parkxml)
    {
        if (strcmp(tagXMLEle(parkxml), "device"))
        {
            parkxml=nextXMLEle(ParkdataXmlRoot, 0);
            continue;
        }
        ap = findXMLAtt(parkxml, "name");
        if (ap && (!strcmp(valuXMLAtt(ap), ParkDeviceName)))
        {
            devicefound = true;
            break;
        }
        parkxml=nextXMLEle(ParkdataXmlRoot, 0);
    }

    if (!devicefound)
        return (char *)"No park data found for this device";

    ParkdeviceXml=parkxml;
    ParkstatusXml = findXMLEle(parkxml, "parkstatus");

    if (ParkstatusXml == NULL)
    {
        return (char *)("Park data invalid or missing.");
    }

    if (!strcmp(pcdataXMLEle(ParkstatusXml), "true"))
        SetParked(true);
    else
        SetParked(false);


    return NULL;
}

bool Parkdata::WriteParkData()
{
    wordexp_t wexp;
    FILE *fp;
    char pcdata[30];

    ParkDeviceName = getDeviceName();

    if (wordexp(Parkdatafile, &wexp, 0))
    {
        wordfree(&wexp);
        DEBUGF( INDI::Logger::DBG_SESSION, "WriteParkData: can not write file %s: Badly formed filename.", Parkdatafile);
        return false;
    }

    if (!(fp=fopen(wexp.we_wordv[0], "w")))
    {
        wordfree(&wexp);
        DEBUGF( INDI::Logger::DBG_SESSION, "WriteParkData: can not write file %s: %s", Parkdatafile, strerror(errno));
        return false;
    }

    if (!ParkdataXmlRoot)
        ParkdataXmlRoot=addXMLEle(NULL, "parkdata");

    if (!ParkdeviceXml)
    {
        ParkdeviceXml=addXMLEle(ParkdataXmlRoot, "device");
        addXMLAtt(ParkdeviceXml, "name", ParkDeviceName);
    }

    if (!ParkstatusXml)
        ParkstatusXml=addXMLEle(ParkdeviceXml, "parkstatus");

    editXMLEle(ParkstatusXml, (IsParked?"true":"false"));

    prXMLEle(fp, ParkdataXmlRoot, 0);
    fclose(fp);

    return true;
}
