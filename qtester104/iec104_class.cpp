/*
 * This software implements an IEC 60870-5-104 protocol tester.
 * Copyright � 2010,2011,2012 Ricardo L. Olsen
 *
 * Disclaimer
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>

#include "iec104_class.h"

using namespace std;

iec104_class::iec104_class()
{
    strncpy( slaveIP, "", 20 );

    Port = 2404;

    msg_supervisory = true;
    seq_order_check = true;
    connectedTCP = false;

    tout_startdtact = -1;
    tout_supervisory = -1;
    tout_testfr = -1;
    tout_gi = -1;
    VS = 0;
    VR = 0;
    TxOk = false;
    masterAddress = 0;
    slaveAddress = 0;
    slaveASDUAddrCmd = 0;
    GIObjectCnt = 0;
    gi_period = 5 * 60 +30;
}

void iec104_class::disableSequenceOrderCheck()
{
    seq_order_check = false;
}

int iec104_class::getPortTCP()
{
    return Port;
}

void iec104_class::setGIPeriod( unsigned period )
{
    gi_period = period;
}

void iec104_class::setPortTCP( unsigned port )
{
    Port = port;
}

void iec104_class::setSecondaryIP( char * ip )
{
    strncpy( slaveIP, ip, 20 );
}

void iec104_class::setSecondaryIP_backup( char * ip )
{
    strncpy( slaveIP_backup, ip, 20 );
}

char * iec104_class::getSecondaryIP()
{
    return slaveIP;
}

char * iec104_class::getSecondaryIP_backup()
{
    return slaveIP_backup;
}

void iec104_class::setSecondaryAddress( int addr )
{
    slaveAddress = addr;
}

void iec104_class::setSecondaryASDUAddress( int addr )
{
    slaveASDUAddrCmd = addr;
}

int iec104_class::getSecondaryAddress()
{
    return slaveAddress;
}

void iec104_class::setPrimaryAddress( int addr )
{
    masterAddress = addr;
}

int iec104_class::getPrimaryAddress()
{
    return masterAddress;
}

void iec104_class::onConnectTCP()
{
    connectedTCP = true;
    TxOk = false;
    VS = 0;
    VR = 0;
    mLog.pushMsg("*** TCP CONNECT!");
    sendStartDTACT();
}

void iec104_class::onDisconnectTCP()
{
    connectedTCP = false;
    tout_startdtact = -1;
    tout_supervisory = -1;
    tout_gi = -1;
    TxOk = false;
    mLog.pushMsg("*** TCP DISCONNECT!");
}

void iec104_class::onTimerSecond()
{
    iec_apdu apdu;

    if (connectedTCP)
    {
        if ( tout_startdtact > 0 )
            tout_startdtact--;
        if ( tout_startdtact==0 )  // timeout of startdtact: retry
            sendStartDTACT();

        if ( tout_gi > 0 )
          {
            tout_gi--;
            if ( tout_gi == 0 )
              {
                solicitGI();
              }
          }

        if (msg_supervisory)
        {
        if (tout_supervisory > 0)
          tout_supervisory--;
        if (tout_supervisory > 0)
          tout_supervisory--;

        if (tout_supervisory == 0)
          {
           tout_supervisory = -1;
           sendSupervisory();
          }
        }
    }

    // if connected and no data received, send TESTFRACT
    if ( connectedTCP && TxOk )
    {
        if ( tout_testfr > 0 )
          {
            tout_testfr--;
            if ( tout_testfr == 0 )
            {
                apdu.start = START;
                apdu.length = 4;
                apdu.NS = TESTFRACT;
                apdu.NR = 0;
                sendTCP((char *)&apdu, 6);
                mLog.pushMsg("T<-- TESTFRACT");
            }
          }
    }

}

void iec104_class::solicitGI()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 0x0E;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = INTERROGATION;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTIVATION;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = slaveAddress;
    wapdu.dados[0] = 0x00;
    wapdu.dados[1] = 0x00;
    wapdu.dados[2] = 0x00;
    wapdu.dados[3] = 0x14;
    sendTCP((char *)&wapdu, 16);
    VS += 2;
    mLog.pushMsg( "T<-- INTERROGATION " );
    tout_gi = gi_retry_time;
}

void iec104_class::confTestCommand()
{
    iec_apdu wapdu;

    wapdu.start = START;
    wapdu.length = 22;
    wapdu.NS = VS;
    wapdu.NR = VR;
    wapdu.asduh.type = C_TS_TA_1;
    wapdu.asduh.num = 1;
    wapdu.asduh.sq = 0;
    wapdu.asduh.cause = ACTCONFIRM;
    wapdu.asduh.t = 0;
    wapdu.asduh.pn = 0;
    wapdu.asduh.oa = masterAddress;
    wapdu.asduh.ca = slaveAddress;
    time_t tm1 = time( NULL );
    tm *agora = localtime( &tm1 );

    wapdu.asdu107.ioa16 = 0;
    wapdu.asdu107.ioa8 = 0;
    wapdu.asdu107.tsc = 0;
    wapdu.asdu107.time.year = agora->tm_year % 100;
    wapdu.asdu107.time.month = agora->tm_mon;
    wapdu.asdu107.time.mday = agora->tm_mday;
    wapdu.asdu107.time.hour = agora->tm_hour;
    wapdu.asdu107.time.min = agora->tm_min;
    wapdu.asdu107.time.msec = agora->tm_sec * 1000;

    sendTCP( (char *)&wapdu, 22+2 );
    VS += 2;

    mLog.pushMsg( "T<-- TEST COMMAND CONF " );
}

void iec104_class::sendStartDTACT()
{
    // send STARTDTACT: enable data transfer
    iec_apdu apdu;
    apdu.start=START;
    apdu.length=4;
    apdu.NS=STARTDTACT;
    apdu.NR=0;
    sendTCP((char *)&apdu, 6);
    mLog.pushMsg("T<-- STARTDTACT");
    tout_startdtact=t1_startdtact;
}

// tcp packet ready to be read from connection with the iec104 slave
void iec104_class::packetReadyTCP()
{
    static bool broken_msg=false;
    static iec_apdu apdu;
    unsigned char * br;
    br = (unsigned char*)&apdu;
    int bytesrec;
    unsigned char byt;
    unsigned char len;

    while (true) {

      if (!broken_msg)
        {
        // look for a START
        do {
            bytesrec=readTCP((char*)br, 1);
            if (bytesrec==0) return;
            byt = br[0];
        } while (byt != START);
        bytesrec=readTCP( (char*)br+1, 1); // length of apdu
        if (bytesrec==0) return;
        }

      len = br[1];
      if ( len < 4 ) // apdu length must be >= 4
        {
        broken_msg=false;
        mLog.pushMsg("R--> ERROR: INVALID FRAME");
        continue;
        }

      bytesrec=readTCP((char*)br+2, len); // read the remaining of the apdu
      if (bytesrec==0)
         {
         mLog.pushMsg("R--> Broken apdu");
         broken_msg=true;
         return;
         }

      if ( apdu.asduh.ca != slaveAddress && apdu.asduh.ca != slaveASDUAddrCmd && len>4 )
        {
        broken_msg=false;
        mLog.pushMsg("R--> ASDU WITH UNEXPECTED ORIGIN! Ignoring...");
        // continue;
        }

      broken_msg=false;

      if (mLog.isLogging())
        {
        char buflog[5000];

        sprintf (buflog, "R--> %03d: ", (int)len+2);
        for (int i=0; i< len+2 && i<25 ; i++) // log up to 25 caracteres
           sprintf (buflog+strlen(buflog), "%02x ", br[i]);
        mLog.pushMsg(buflog);
        }

      userprocAPDU( &apdu, len + 2 );
      parseAPDU( &apdu, len + 2 );
      break;
      }

/*
    char buf[5000];
    buf[0]=0;

    sprintf(buf+strlen(buf), "R--> ");
    bytesrec=readTCP((char*)br,2); // start and length
    if ( br[0]== START) // valid frames must begin with START
    {
        bytesrec=readTCP((char*)br+2,br[1]); // read the remaining of the frame
        sprintf (buf+strlen(buf), "%03d: ", bytesrec+2);
        int i=0;
        for (; i< bytesrec+2 && i<25 ; i++) // prints up to 25 chars
            sprintf (buf+strlen(buf), "%02x ", br[i]);
        mLog.pushMsg(buf);
        buf[0]=0;
        parseAPDU(&apdu, bytesrec+2);
    }
    else
    {
        mLog.pushMsg("R--> ERROR: INVALID FRAME");
        bytesrec=readTCP((char*)br,sizeof(apdu)); // consider garbage the remaining data from the frame
    }
*/
}

void iec104_class::parseAPDU(iec_apdu * papdu, int sz, bool accountandrespond)
{
    iec_apdu wapdu;      // buffer to assemble apdu to send
    string qs, qsa;
    stringstream oss;
    unsigned short VR_NEW;
    
    if ( papdu->start!=START )
    { // invalid frame
        mLog.pushMsg("R--> ERROR: NO START IN FRAME");
        return;
    }

    if ( papdu->asduh.ca != slaveAddress && papdu->asduh.ca != slaveASDUAddrCmd && sz>6)
    { // invalid frame
        mLog.pushMsg("R--> ASDU WITH UNEXPECTED ORIGIN! Ignoring...");
        return;
    }

    if (sz==6)
    { // Control messages
        if ( accountandrespond )
        switch ( papdu->NS )
        {
        case STARTDTACT:
            mLog.pushMsg("     STARTDTACT");
            wapdu.start=START;
            wapdu.length=4;
            wapdu.NS=STARTDTCON;
            wapdu.NR=0;
            sendTCP((char *)&wapdu, 6);
            mLog.pushMsg("T<-- STARTDTCON");
            break;
            
        case TESTFRACT:
            mLog.pushMsg("     TESTFRACT");
            wapdu.start=START;
            wapdu.length=4;
            wapdu.NS=TESTFRCON;
            wapdu.NR=0;
            sendTCP((char *)&wapdu, 6);
            mLog.pushMsg("T<-- TESTFRCON");
            break;
            
        case STARTDTCON:
            mLog.pushMsg("     STARTDTCON");
            tout_startdtact=-1; // flag confirmation of STARTDT, not to timeout
            TxOk=true;
            tout_gi=15; // request GI when communication starts
            break;
            
        case STOPDTACT:
            mLog.pushMsg("     STOPDTACT");
            // only slave responds
            break;
            
        case STOPDTCON:
            mLog.pushMsg("     STOPDTCON");
            // do what?
            break;
            
        case TESTFRCON:
            mLog.pushMsg("     TESTFRCON");
            // do what?
            break;
            
        case SUPERVISORY:
            mLog.pushMsg("     SUPERVISORY");
            // do what?
            break;

        default: // error
            mLog.pushMsg("     ERROR: UNKNOWN CONTROL MESSAGE");
            break;
        }
        
    }
    else
    { // data message

        if ( accountandrespond )
        {
        VR_NEW = (papdu->NS & 0xFFFE);

        if ( VR_NEW != VR )
          {
            // sequence error, must close and reopen connection
            mLog.pushMsg("*** SEQUENCE ERROR! **************************");
            if ( seq_order_check )
              {
              disconnectTCP();
              return;
              }
          }

        VR = VR_NEW + 2;
        }

        oss.str("");
        oss << "     CA "
                << (unsigned)papdu->asduh.ca
                << " TYPE "
                << (unsigned)papdu->asduh.type
                << " CAUSE "
                << (int) papdu->asduh.cause
                << " SQ "
                << (unsigned)papdu->asduh.sq
                << " NUM "
                << (unsigned)papdu->asduh.num;
        mLog.pushMsg((char*)oss.str().c_str());
        
        switch (papdu->asduh.type)
        {
        case M_SP_NA_1:	// 1: DIGITAL SINGLE
            {
                unsigned int addr24=0;
                iec_type1 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq1.obj[i];
                        if (i==0)
                          addr24 = papdu->sq1.ioa16 + ((unsigned)papdu->sq1.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq1[i].obj;
                        addr24 = papdu->nsq1[i].ioa16 + ((unsigned)papdu->nsq1[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->sp;
                      piecarr[i].sp=pobj->sp;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_DP_NA_1:	// 3: DIGITAL DOUBLE
            {
                unsigned int addr24=0;
                iec_type3 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq3.obj[i];
                        if (i==0)
                          addr24 = papdu->sq3.ioa16 + ((unsigned)papdu->sq3.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq3[i].obj;
                        addr24 = papdu->nsq3[i].ioa16 + ((unsigned)papdu->nsq3[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->dp;
                      piecarr[i].dp=pobj->dp;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ST_NA_1:	// 5: step position
            {
                unsigned int addr24=0;
                iec_type5 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq5.obj[i];
                        if (i==0)
                          addr24 = papdu->sq5.ioa16 + ((unsigned)papdu->sq5.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq5[i].obj;
                        addr24 = papdu->nsq5[i].ioa16 + ((unsigned)papdu->nsq5[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].t=pobj->t;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ME_NA_1:	// 9: ANALOGIC NORMALIZED
            {
                unsigned int addr24=0;
                iec_type9 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq9.obj[i];
                        if (i==0)
                          addr24 = papdu->sq9.ioa16 + ((unsigned)papdu->sq9.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq9[i].obj;
                        addr24 = papdu->nsq9[i].ioa16 + ((unsigned)papdu->nsq9[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ME_NB_1:	// 11: ANALOGIC CONVERTED
            {
                unsigned int addr24=0;
                iec_type11 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq11.obj[i];
                        if (i==0)
                          addr24 = papdu->sq11.ioa16 + ((unsigned)papdu->sq11.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq11[i].obj;
                        addr24 = papdu->nsq11[i].ioa16 + ((unsigned)papdu->nsq11[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ME_NC_1:	// 13: ANALOGIC FLOATING POINT
            {
                unsigned int addr24=0;
                iec_type13 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq13.obj[i];
                        if (i==0)
                          addr24 = papdu->sq13.ioa16 + ((unsigned)papdu->sq13.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq13[i].obj;
                        addr24 = papdu->nsq13[i].ioa16 + ((unsigned)papdu->nsq13[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_SP_TB_1:	// 30:  DIGITAL SINGLE WITH LONG TIME TAG
            {
                unsigned int addr24=0;
                iec_type30 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq30.obj[i];
                        if (i==0)
                          addr24 = papdu->sq30.ioa16 + ((unsigned)papdu->sq30.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq30[i].obj;
                        addr24 = papdu->nsq30[i].ioa16 + ((unsigned)papdu->nsq30[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->sp;
                      piecarr[i].sp=pobj->sp;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_DP_TB_1:	// 31: DIGITAL DOUBLE WITH LONG TIME TAG
            {
                unsigned int addr24=0;
                iec_type31 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq31.obj[i];
                        if (i==0)
                          addr24 = papdu->sq31.ioa16 + ((unsigned)papdu->sq31.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq31[i].obj;
                        addr24 = papdu->nsq31[i].ioa16 + ((unsigned)papdu->nsq31[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->dp;
                      piecarr[i].dp=pobj->dp;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ST_TB_1:	// 32: TAP WITH TIME TAG
            {
                unsigned int addr24=0;
                iec_type32 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq32.obj[i];
                        if (i==0)
                          addr24 = papdu->sq32.ioa16 + ((unsigned)papdu->sq32.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq32[i].obj;
                        addr24 = papdu->nsq32[i].ioa16 + ((unsigned)papdu->nsq32[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].t=pobj->t;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_BO_NA_1:	//7
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            break;
        case M_BO_TB_1:	//33
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            break;
        case M_ME_TD_1:	//34 MEASURED VALUE, NORMALIZED WITH TIME TAG
            {
                unsigned int addr24=0;
                iec_type34 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq34.obj[i];
                        if (i==0)
                          addr24 = papdu->sq34.ioa16 + ((unsigned)papdu->sq34.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq34[i].obj;
                        addr24 = papdu->nsq34[i].ioa16 + ((unsigned)papdu->nsq34[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ME_TE_1:	//35 MEASURED VALUE, SCALED WITH TIME TAG
            {
                unsigned int addr24=0;
                iec_type35 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq35.obj[i];
                        if (i==0)
                          addr24 = papdu->sq35.ioa16 + ((unsigned)papdu->sq35.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq35[i].obj;
                        addr24 = papdu->nsq35[i].ioa16 + ((unsigned)papdu->nsq35[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case M_ME_TF_1:	//36 MEASURED VALUE, FLOATING POINT WITH TIME TAG
            {
                unsigned int addr24=0;
                iec_type36 *pobj;
                iec_obj *piecarr = new iec_obj [papdu->asduh.num];
                if (papdu->asduh.cause==20)
                   GIObjectCnt+=papdu->asduh.num;

                for ( int i=0; i<papdu->asduh.num; i++ )
                   {
                     if ( papdu->asduh.sq )
                        {
                        pobj =  &papdu->sq36.obj[i];
                        if (i==0)
                          addr24 = papdu->sq36.ioa16 + ((unsigned)papdu->sq36.ioa8 << 16);
                        else
                          addr24++;
                        }
                     else
                        {
                        pobj =  &papdu->nsq36[i].obj;
                        addr24 = papdu->nsq36[i].ioa16 + ((unsigned)papdu->nsq36[i].ioa8 << 16);
                        }

                      piecarr[i].address=addr24;
                      piecarr[i].ca=papdu->asduh.ca;
                      piecarr[i].cause=papdu->asduh.cause;
                      piecarr[i].pn=papdu->asduh.pn;
                      piecarr[i].type=papdu->asduh.type;
                      piecarr[i].value=pobj->mv;
                      piecarr[i].ov=pobj->ov;
                      piecarr[i].bl=pobj->bl;
                      piecarr[i].nt=pobj->nt;
                      piecarr[i].sb=pobj->sb;
                      piecarr[i].iv=pobj->iv;
                      piecarr[i].timetag.mday=pobj->time.mday;
                      piecarr[i].timetag.month=pobj->time.month;
                      piecarr[i].timetag.year=pobj->time.year;
                      piecarr[i].timetag.hour=pobj->time.hour;
                      piecarr[i].timetag.min=pobj->time.min;
                      piecarr[i].timetag.msec=pobj->time.msec;
                      piecarr[i].timetag.iv=pobj->time.iv;
                   }
                dataIndication(piecarr, papdu->asduh.num);
                delete[] piecarr;
            }
            break;
        case C_SC_NA_1: // SINGLE COMMAND
            {
            iec_type45 *pobj;
            pobj =  &papdu->nsq45.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SINGLE COMMAND ADDRESS "
                    << (unsigned)papdu->nsq45.ioa16 + ((unsigned)papdu->nsq45.ioa8 << 16)
                    << " SCS "
                    << (unsigned)pobj->scs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq45.ioa16 + ((unsigned)papdu->nsq45.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.scs = pobj->scs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;
        case C_DC_NA_1: // DOUBLE COMMAND
            {
            iec_type46 *pobj;
            pobj =  &papdu->nsq46.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "DOUBLE COMMAND ADDRESS "
                    << (unsigned)papdu->nsq46.ioa16 + ((unsigned)papdu->nsq46.ioa8 << 16)
                    << " DCS "
                    << (unsigned)pobj->dcs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq46.ioa16 + ((unsigned)papdu->nsq46.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.dcs = pobj->dcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;
        case C_RC_NA_1: // REG.STEP COMMAND
            {
            iec_type47 *pobj;
            pobj =  &papdu->nsq47.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "STEP REG. COMMAND ADDRESS "
                    << (unsigned)papdu->nsq47.ioa16 + ((unsigned)papdu->nsq47.ioa8 << 16)
                    << " RCS "
                    << (unsigned)pobj->rcs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());
            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq47.ioa16 + ((unsigned)papdu->nsq47.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.rcs = pobj->rcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SC_TA_1: // SINGLE COMMAND WITH TIME
            {
            iec_type58 *pobj;
            pobj =  &papdu->nsq58.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SINGLE COMMAND ADDRESS "
                    << (unsigned)papdu->nsq58.ioa16 + ((unsigned)papdu->nsq58.ioa8 << 16)
                    << " SCS "
                    << (unsigned)pobj->scs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq58.ioa16 + ((unsigned)papdu->nsq58.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.scs = pobj->scs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;
        case C_DC_TA_1: // DOUBLE COMMAND WITH TIME
            {
            iec_type59 *pobj;
            pobj =  &papdu->nsq59.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "DOUBLE COMMAND ADDRESS "
                    << (unsigned)papdu->nsq59.ioa16 + ((unsigned)papdu->nsq59.ioa8 << 16)
                    << " DCS "
                    << (unsigned)pobj->dcs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq59.ioa16 + ((unsigned)papdu->nsq59.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.dcs = pobj->dcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;
        case C_RC_TA_1: // REG. STEP COMMAND WITH TIME
            {
            iec_type60 *pobj;
            pobj =  &papdu->nsq60.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "STEP REG. COMMAND ADDRESS "
                    << (unsigned)papdu->nsq60.ioa16 + ((unsigned)papdu->nsq60.ioa8 << 16)
                    << " RCS "
                    << (unsigned)pobj->rcs
                    << " QU "
                    << (int) pobj->qu
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());
            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq60.ioa16 + ((unsigned)papdu->nsq60.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.rcs = pobj->rcs;
            iobj.qu = pobj->qu;
            iobj.se = pobj->se;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_NA_1: // NORMALISED COMMAND
            {
            iec_type48 *pobj;
            pobj =  &papdu->nsq48.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "NORMALISED COMMAND ADDRESS "
                    << (unsigned)papdu->nsq48.ioa16 + ((unsigned)papdu->nsq48.ioa8 << 16)
                    << " VAL "
                    << pobj->nva
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq48.ioa16 + ((unsigned)papdu->nsq48.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->nva;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_TA_1: // NORMALISED COMMAND WITH TIME
            {
            iec_type61 *pobj;
            pobj =  &papdu->nsq61.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "NORMALISED COMMAND ADDRESS "
                    << (unsigned)papdu->nsq61.ioa16 + ((unsigned)papdu->nsq61.ioa8 << 16)
                    << " VAL "
                    << pobj->nva
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq61.ioa16 + ((unsigned)papdu->nsq61.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->nva;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_NB_1: // SCALED COMMAND
            {
            iec_type49 *pobj;
            pobj =  &papdu->nsq49.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SCALED COMMAND ADDRESS "
                    << (unsigned)papdu->nsq49.ioa16 + ((unsigned)papdu->nsq49.ioa8 << 16)
                    << " VAL "
                    << pobj->sva
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq49.ioa16 + ((unsigned)papdu->nsq49.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->sva;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_TB_1: // SCALED COMMAND WITH TIME
            {
            iec_type62 *pobj;
            pobj =  &papdu->nsq62.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "SCALED COMMAND ADDRESS "
                    << (unsigned)papdu->nsq62.ioa16 + ((unsigned)papdu->nsq62.ioa8 << 16)
                    << " VAL "
                    << pobj->sva
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq62.ioa16 + ((unsigned)papdu->nsq62.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->sva;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_NC_1: // FLOAT COMMAND
            {
            iec_type50 *pobj;
            pobj =  &papdu->nsq50.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "FLOAT COMMAND ADDRESS "
                    << (unsigned)papdu->nsq50.ioa16 + ((unsigned)papdu->nsq50.ioa8 << 16)
                    << " VAL "
                    << pobj->r32
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq50.ioa16 + ((unsigned)papdu->nsq50.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->r32;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case C_SE_TC_1: // FLOAT COMMAND WITH TIME
            {
            iec_type63 *pobj;
            pobj =  &papdu->nsq63.obj;

            oss.str("");
            oss << "     ";
            if (papdu->asduh.cause==ACTCONFIRM)
                oss << "ACTIVATION CONFIRMATION ";
            else
            if (papdu->asduh.cause==ACTTERM)
                oss << "ACTIVATION TERMINATION ";
            if (papdu->asduh.pn==POSITIVE)
                oss << "POSITIVE ";
            else
                oss << "NEGATIVE ";
            oss << "FLOAT COMMAND ADDRESS "
                    << (unsigned)papdu->nsq63.ioa16 + ((unsigned)papdu->nsq63.ioa8 << 16)
                    << " VAL "
                    << pobj->r32
                    << " QL "
                    << (int) pobj->ql
                    << " SE "
                    << (unsigned)pobj->se;
            mLog.pushMsg((char*)oss.str().c_str());

            // send indication to user
            iec_obj iobj;
            iobj.address = papdu->nsq63.ioa16 + ((unsigned)papdu->nsq63.ioa8 << 16);
            iobj.cause = papdu->asduh.cause;
            iobj.pn = papdu->asduh.pn;
            iobj.type = papdu->asduh.type;
            iobj.qu = 0;
            iobj.se = pobj->se;
            iobj.value = pobj->r32;
            if ( papdu->asduh.cause == ACTCONFIRM )
              commandActConfIndication( &iobj );
            else
            if ( papdu->asduh.cause == ACTTERM )
              commandActTermIndication( &iobj );
            }
            break;

        case M_EI_NA_1:	//70
            mLog.pushMsg("R--> END OF INITIALIZATION");
            break;
        case INTERROGATION: // GI
            tout_gi = gi_period; // restart count to next GI
            if (papdu->asduh.cause==ACTCONFIRM)
            {
                GIObjectCnt=0;
                mLog.pushMsg("     INTERROGATION ACT CON ------------------------------------------------------------------------");
                interrogationActConfIndication();
            }
            else
                if (papdu->asduh.cause==ACTTERM)
                {
                mLog.pushMsg("     INTERROGATION ACT TERM ------------------------------------------------------------------------");
                oss.str("");
                oss << "    Total objects in GI: "
                        << GIObjectCnt;
                mLog.pushMsg((char*)oss.str().c_str());

                interrogationActTermIndication();
                }
            else
                mLog.pushMsg("     INTERROGATION");
            break;
        case C_TS_TA_1: // 107
            oss.str("");
            oss << "     TEST COMMAND COM TAG";
            mLog.pushMsg((char*)oss.str().c_str());

            if (papdu->asduh.cause==ACTIVATION)
            {
                // iec_type107 * ptype107;
                // ptype107=(iec_type107 *)papdu->dados;
                confTestCommand();
            }
            break;
        case C_CS_NA_1: // 103
            oss.str("");
            oss << "     CLOCK SYNC COMMAND";
            mLog.pushMsg((char*)oss.str().c_str());
            // iec_type103 * ptype103;
            // ptype103=(iec_type103 *)papdu->dados;
            break;
        default:
            mLog.pushMsg("!!! TYPE NOT IMPLEMENTED");
            break;
        }

        if ( accountandrespond )
        {

            tout_testfr=t3_testfr;

            if (msg_supervisory)
            {
                // will wait t2 seconds or n messages to send supervisory window control
                if (tout_supervisory<0)
                    tout_supervisory=t2_supervisory;

                if (tout_supervisory>0)
                    tout_supervisory--;

                if (tout_supervisory==0)
                {
                    tout_supervisory=-1;
                    sendSupervisory();
                }
            }
            else
                sendSupervisory();
        }
    }
}

void iec104_class::sendSupervisory()
{
stringstream oss;
iec_apdu apdu;

apdu.start=START;
apdu.length=4;
apdu.NS=SUPERVISORY;
apdu.NR=VR;
sendTCP((char *)&apdu, 6);

oss.str("");
oss.setf ( ios::hex, ios::basefield );
oss << "T<-- SUPERVISORY " << VR;
mLog.pushMsg((char*)(oss.str().c_str()));
}

bool iec104_class::sendCommand(iec_obj *obj)
{
iec_apdu apducmd;
time_t tm1=time(NULL);
tm *agora=localtime(&tm1);
stringstream oss;

obj->cause = ACTIVATION;

if ( obj->ca == 0 )
  obj->ca = slaveAddress;

switch (obj->type)
  {
  case C_SC_NA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq45);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq45.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq45.ioa8 = obj->address >> 16;
    apducmd.nsq45.obj.scs = obj->scs;
    apducmd.nsq45.obj.res = 0;
    apducmd.nsq45.obj.qu = obj->qu;
    apducmd.nsq45.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- SINGLE COMMAND ADDRESS "
            << (unsigned)obj->address
            << " SCS "
            << (unsigned)obj->scs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());

    break;
  case C_DC_NA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq46);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq46.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq46.ioa8 = obj->address >> 16;
    apducmd.nsq46.obj.dcs = obj->dcs;
    apducmd.nsq46.obj.qu = obj->qu;
    apducmd.nsq46.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- DOUBLE COMMAND ADDRESS "
            << (unsigned)obj->address
            << " DCS "
            << (unsigned)obj->dcs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_RC_NA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq47);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq47.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq47.ioa8 = obj->address >> 16;
    apducmd.nsq47.obj.rcs = obj->rcs;
    apducmd.nsq47.obj.qu = obj->qu;
    apducmd.nsq47.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;
    oss.str("");
    oss << "T<-- STEP REG. COMMAND ADDRESS "
            << (unsigned)obj->address
            << " RCS "
            << (unsigned)obj->rcs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SC_TA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq58);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq58.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq58.ioa8 = obj->address >> 16;
    apducmd.nsq58.obj.scs = obj->scs;
    apducmd.nsq58.obj.res = 0;
    apducmd.nsq58.obj.qu = obj->qu;
    apducmd.nsq58.obj.se = obj->se;
    apducmd.nsq58.obj.time.year=agora->tm_year%100;
    apducmd.nsq58.obj.time.month=agora->tm_mon+1;
    apducmd.nsq58.obj.time.mday=agora->tm_mday;
    apducmd.nsq58.obj.time.hour=agora->tm_hour;
    apducmd.nsq58.obj.time.min=agora->tm_min;
    apducmd.nsq58.obj.time.hour=agora->tm_hour;
    apducmd.nsq58.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq58.obj.time.iv=0;
    apducmd.nsq58.obj.time.su=0;
    apducmd.nsq58.obj.time.wday=agora->tm_wday;
    apducmd.nsq58.obj.time.res1=0;
    apducmd.nsq58.obj.time.res2=0;
    apducmd.nsq58.obj.time.res3=0;
    apducmd.nsq58.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- SINGLE COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " SCS "
            << (unsigned)obj->scs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());

    break;
  case C_DC_TA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq59);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq59.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq59.ioa8 = obj->address >> 16;
    apducmd.nsq59.obj.dcs = obj->dcs;
    apducmd.nsq59.obj.qu = obj->qu;
    apducmd.nsq59.obj.se = obj->se;
    apducmd.nsq59.obj.time.year=agora->tm_year%100;
    apducmd.nsq59.obj.time.month=agora->tm_mon+1;
    apducmd.nsq59.obj.time.mday=agora->tm_mday;
    apducmd.nsq59.obj.time.hour=agora->tm_hour;
    apducmd.nsq59.obj.time.min=agora->tm_min;
    apducmd.nsq59.obj.time.hour=agora->tm_hour;
    apducmd.nsq59.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq59.obj.time.iv=0;
    apducmd.nsq59.obj.time.su=0;
    apducmd.nsq59.obj.time.wday=agora->tm_wday;
    apducmd.nsq59.obj.time.res1=0;
    apducmd.nsq59.obj.time.res2=0;
    apducmd.nsq59.obj.time.res3=0;
    apducmd.nsq59.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- DOUBLE COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " DCS "
            << (unsigned)obj->dcs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_RC_TA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq60);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq60.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq60.ioa8 = obj->address >> 16;
    apducmd.nsq60.obj.rcs = obj->rcs;
    apducmd.nsq60.obj.qu = obj->qu;
    apducmd.nsq60.obj.se = obj->se;
    apducmd.nsq60.obj.time.year=agora->tm_year%100;
    apducmd.nsq60.obj.time.month=agora->tm_mon+1;
    apducmd.nsq60.obj.time.mday=agora->tm_mday;
    apducmd.nsq60.obj.time.hour=agora->tm_hour;
    apducmd.nsq60.obj.time.min=agora->tm_min;
    apducmd.nsq60.obj.time.hour=agora->tm_hour;
    apducmd.nsq60.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq60.obj.time.iv=0;
    apducmd.nsq60.obj.time.su=0;
    apducmd.nsq60.obj.time.wday=agora->tm_wday;
    apducmd.nsq60.obj.time.res1=0;
    apducmd.nsq60.obj.time.res2=0;
    apducmd.nsq60.obj.time.res3=0;
    apducmd.nsq60.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;
    oss.str("");
    oss << "T<-- STEP REG. COMMAND W/TIME ADDRESS "
            << (unsigned)obj->address
            << " RCS "
            << (unsigned)obj->rcs
            << " CA "
            << obj->ca
            << " QU "
            << (int) obj->qu
            << " SE "
            << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_NA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq48);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq48.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq48.ioa8 = obj->address >> 16;
    apducmd.nsq48.obj.nva = (short)obj->value;
    apducmd.nsq48.obj.ql = 0;
    apducmd.nsq48.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- NORMALISED COMMAND ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << (short)obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_TA_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq61);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq61.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq61.ioa8 = obj->address >> 16;
    apducmd.nsq61.obj.nva = (short)obj->value;
    apducmd.nsq61.obj.ql = 0;
    apducmd.nsq61.obj.se = obj->se;
    apducmd.nsq61.obj.time.year=agora->tm_year%100;
    apducmd.nsq61.obj.time.month=agora->tm_mon+1;
    apducmd.nsq61.obj.time.mday=agora->tm_mday;
    apducmd.nsq61.obj.time.hour=agora->tm_hour;
    apducmd.nsq61.obj.time.min=agora->tm_min;
    apducmd.nsq61.obj.time.hour=agora->tm_hour;
    apducmd.nsq61.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq61.obj.time.iv=0;
    apducmd.nsq61.obj.time.su=0;
    apducmd.nsq61.obj.time.wday=agora->tm_wday;
    apducmd.nsq61.obj.time.res1=0;
    apducmd.nsq61.obj.time.res2=0;
    apducmd.nsq61.obj.time.res3=0;
    apducmd.nsq61.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- NORMALISED COMMAND W/TIME ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << (short)obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_NB_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq49);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq49.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq49.ioa8 = obj->address >> 16;
    apducmd.nsq49.obj.sva = (short)obj->value;
    apducmd.nsq49.obj.ql = 0;
    apducmd.nsq49.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- SCALED COMMAND ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << (short)obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_TB_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq62);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq62.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq62.ioa8 = obj->address >> 16;
    apducmd.nsq62.obj.sva = (short)obj->value;
    apducmd.nsq62.obj.ql = 0;
    apducmd.nsq62.obj.se = obj->se;
    apducmd.nsq62.obj.time.year=agora->tm_year%100;
    apducmd.nsq62.obj.time.month=agora->tm_mon+1;
    apducmd.nsq62.obj.time.mday=agora->tm_mday;
    apducmd.nsq62.obj.time.hour=agora->tm_hour;
    apducmd.nsq62.obj.time.min=agora->tm_min;
    apducmd.nsq62.obj.time.hour=agora->tm_hour;
    apducmd.nsq62.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq62.obj.time.iv=0;
    apducmd.nsq62.obj.time.su=0;
    apducmd.nsq62.obj.time.wday=agora->tm_wday;
    apducmd.nsq62.obj.time.res1=0;
    apducmd.nsq62.obj.time.res2=0;
    apducmd.nsq62.obj.time.res3=0;
    apducmd.nsq62.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- SCALED COMMAND W/TIME ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << (short)obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_NC_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq50);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq50.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq50.ioa8 = obj->address >> 16;
    apducmd.nsq50.obj.r32 = obj->value;
    apducmd.nsq50.obj.ql = 0;
    apducmd.nsq50.obj.se = obj->se;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- FLOAT COMMAND ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_SE_TC_1:
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.nsq63);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.nsq63.ioa16 = obj->address & 0x0000FFFF;
    apducmd.nsq63.ioa8 = obj->address >> 16;
    apducmd.nsq63.obj.r32 = obj->value;
    apducmd.nsq63.obj.ql = 0;
    apducmd.nsq63.obj.se = obj->se;
    apducmd.nsq63.obj.time.year=agora->tm_year%100;
    apducmd.nsq63.obj.time.month=agora->tm_mon+1;
    apducmd.nsq63.obj.time.mday=agora->tm_mday;
    apducmd.nsq63.obj.time.hour=agora->tm_hour;
    apducmd.nsq63.obj.time.min=agora->tm_min;
    apducmd.nsq63.obj.time.hour=agora->tm_hour;
    apducmd.nsq63.obj.time.msec=agora->tm_sec*1000;
    apducmd.nsq63.obj.time.iv=0;
    apducmd.nsq63.obj.time.su=0;
    apducmd.nsq63.obj.time.wday=agora->tm_wday;
    apducmd.nsq63.obj.time.res1=0;
    apducmd.nsq63.obj.time.res2=0;
    apducmd.nsq63.obj.time.res3=0;
    apducmd.nsq63.obj.time.res4=0;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- SCALED COMMAND W/TIME ADDRESS "
        << (unsigned)obj->address
        << " VAL "
        << obj->value
        << " CA "
        << obj->ca
        << " SE "
        << (unsigned)obj->se;
    mLog.pushMsg((char*)oss.str().c_str());
    break;
  case C_CS_NA_1: // Clock Sync
    apducmd.start = START;
    apducmd.length = sizeof(apducmd.NS)+sizeof(apducmd.NR)+sizeof(apducmd.asduh)+sizeof(apducmd.asdu103);
    apducmd.NS = VS;
    apducmd.NR = VR;
    apducmd.asduh.type = obj->type;
    apducmd.asduh.num = 1;
    apducmd.asduh.sq = 0;
    apducmd.asduh.cause = obj->cause;
    apducmd.asduh.t = 0;
    apducmd.asduh.pn = 0;
    apducmd.asduh.oa = masterAddress;
    apducmd.asduh.ca = obj->ca;
    apducmd.asdu103.ioa8 = 0;
    apducmd.asdu103.ioa16 = 0;
    apducmd.asdu103.time = obj->timetag;
    sendTCP( (char *)&apducmd, apducmd.length + sizeof(apducmd.start) + sizeof(apducmd.length) );
    VS+=2;

    oss.str("");
    oss << "T<-- CLOCK SYNC COMMAND "
        << " CA "
        << obj->ca;
    mLog.pushMsg((char*)oss.str().c_str());
    break;

  default:
    return false;
  }

return true;
}

