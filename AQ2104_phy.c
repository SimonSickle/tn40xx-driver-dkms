#include "tn40.h"
#include "AQ2104_phy.h"



#define   mailboxWrite  (0xC000)
#define   phyImageHeaderContentOffset (0x300)
#define   r1 (0x1E)

//#define AQ_VERBOSE

#ifdef AQ_VERBOSE
#define printf ERR
#endif

#define LINK_LOOP_MAX   			(25)



void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//----------------------------

__init int AQ2104_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{
    u16  val, val1,val2;
    int             rVal = 0;
    u32             i;
    void __iomem 	*regs=priv->pBdxRegs;
    u8 *image=AQ2104_phy_firmware;  
    u32 primaryHeaderPtr;
    u32 primaryIramPtr,primaryIramSize,primaryDramPtr,primaryDramSize;
    u32 byteSize,  dWordSize,bytePointer; 
    u16 msw=0,lsw=0, sd, sv;
    int phy_id;


   ENTER;
    phy_id  = bdx_mdio_read(regs, 1, port, 0x0003); // PHY_ID_LOW
    val1  = bdx_mdio_read(regs, r1, port, 0xC441); //???


    BDX_MDIO_WRITE(r1,0xC452,1); //NVR Daisy Chain Disable 
    BDX_MDIO_WRITE(r1,0xC441,0); 

    val=0;//bdx_mdio_read(regs, r1, port,0xC001);
    val |= 1;    //upRunStall
    val |= 1<<6; //upRunStallOverride
    BDX_MDIO_WRITE(r1,0xC001,val);


  /*--- Read the segment addresses and sizes -----------------*/
  primaryHeaderPtr = (((image[0x9] & 0x0F) << 8) | image[0x8]) << 12;

  primaryIramPtr =  (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x4 + 2] << 16) |
                    (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x4 + 1] << 8)  |
                     image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x4];
  primaryIramSize = (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x7 + 2] << 16) |
                    (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x7 + 1] << 8)  |
                     image[primaryHeaderPtr + phyImageHeaderContentOffset + 0x7];
  primaryDramPtr =  (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xA + 2] << 16) |
                    (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xA + 1] << 8)  |
                     image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xA];
  primaryDramSize = (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xD + 2] << 16) |
                    (image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xD + 1] << 8)  |
                     image[primaryHeaderPtr + phyImageHeaderContentOffset + 0xD];

  if ( 1 /*AQ_DEVICE_HHD == port->device*/ )  {
    primaryIramPtr += primaryHeaderPtr;
    primaryDramPtr += primaryHeaderPtr;
  }

#ifdef AQ_VERBOSE
  printf ("\nSegment Addresses and Sizes as read from the PHY ROM image header:\n\n");
  printf ("Primary Iram Address: 0x%x\n", primaryIramPtr);
  printf ("Primary Iram Size: 0x%x\n", primaryIramSize);
  printf ("Primary Dram Address: 0x%x\n", primaryDramPtr);
  printf ("Primary Dram Size: 0x%x\n\n", primaryDramSize);
#endif

  /*-- Load IRAM and DRAM ----------------------*/

  /* clear the mailbox CRC */
  BDX_MDIO_WRITE(r1,0x200,0x1000); /*resetUpMailboxCrc*/
  BDX_MDIO_WRITE(r1,0x200,0x0000); /*resetUpMailboxCrc*/
  

  /* load the IRAM */
#ifdef AQ_VERBOSE
  printf ("Loading IRAM:\n");
#endif

//#define AQ_IRAM_BASE_ADDRESS 0x40000000

  BDX_MDIO_WRITE(r1,0x202,0x4000); //msw
  BDX_MDIO_WRITE(r1,0x203,0x0000); //lsw


  /* set block size so that there are from 0-3 bytes remaining */
  byteSize = primaryIramSize;
  dWordSize = byteSize >> 2;

  bytePointer = primaryIramPtr;
  for (i = 0; i < dWordSize; i++)
  {
    /* write 4 bytes of data */
    lsw = (image[bytePointer+1] << 8) | image[bytePointer];
    bytePointer += 2;
    msw = (image[bytePointer+1] << 8) | image[bytePointer];
    bytePointer += 2;


    BDX_MDIO_WRITE(r1,0x204,msw);
    BDX_MDIO_WRITE(r1,0x205,lsw);
    val = bdx_mdio_read(regs, r1, port, 0x200);
    if (val & (1<<8)){ 
#ifdef AQ_VERBOSE
  printf ("mailbox Busy\n");
#endif
    } 
    BDX_MDIO_WRITE(r1,0x200,mailboxWrite);
  }
  
  
  /* Note: this final write right-justifies non-dWord data in the final dWord */
  if(byteSize & 0x3){ 
	  switch (byteSize & 0x3)
	  {
	    case 0x1:
	      /* write 1 byte of data */
	      lsw = image[bytePointer++];
	      msw = 0x0000;
	      break;

	    case 0x2:
	      /* write 2 bytes of data */
	      lsw = (image[bytePointer+1] << 8) | image[bytePointer];
	      bytePointer += 2;
	      msw = 0x0000;
	      break;

	    case 0x3:
	      /* write 3 bytes of data */
	      lsw = (image[bytePointer+1] << 8) | image[bytePointer];
	      bytePointer += 2;
	      msw = image[bytePointer++];
	      break;
	  }
	  BDX_MDIO_WRITE(r1,0x204,msw);
	  BDX_MDIO_WRITE(r1,0x205,lsw);
	  BDX_MDIO_WRITE(r1,0x200,mailboxWrite);
  }
  
  /* load the DRAM */
#ifdef AQ_VERBOSE
   printf ("Last IRAM data lsw=0x%x msw=0x%x \n",lsw, msw);
   msw = bdx_mdio_read(regs, r1, port, 0x202);
   lsw = bdx_mdio_read(regs, r1, port, 0x203);
   printf ("Last+1 IRAM address lsw=0x%x msw=0x%x \n",lsw, msw);
   printf ("Loading DRAM:\n");
#endif  

//#define AQ_DRAM_BASE_ADDRESS 0x3FFE0000
  BDX_MDIO_WRITE(r1,0x202,0x3FFE); //msw
  BDX_MDIO_WRITE(r1,0x203,0x0000); //lsw



  /* set block size so that there are from 0-3 bytes remaining */
  byteSize = primaryDramSize;
  dWordSize = byteSize >> 2;

  bytePointer = primaryDramPtr;
  for (i = 0; i < dWordSize; i++)
  {
    /* write 4 bytes of data */
    lsw = (image[bytePointer+1] << 8) | image[bytePointer];
    bytePointer += 2;
    msw = (image[bytePointer+1] << 8) | image[bytePointer];
    bytePointer += 2;
    BDX_MDIO_WRITE(r1,0x204,msw);
    BDX_MDIO_WRITE(r1,0x205,lsw);
    BDX_MDIO_WRITE(r1,0x200,mailboxWrite);
  }
  
  
  /* Note: this final write right-justifies non-dWord data in the final dWord */
  if(byteSize & 0x3){ 
	  switch (byteSize & 0x3)
	  {
	    case 0x1:
	      /* write 1 byte of data */
	      lsw = image[bytePointer++];
	      msw = 0x0000;
	      break;

	    case 0x2:
	      /* write 2 bytes of data */
	      lsw = (image[bytePointer+1] << 8) | image[bytePointer];
	      bytePointer += 2;
	      msw = 0x0000;
	      break;

	    case 0x3:
	      /* write 3 bytes of data */
	      lsw = (image[bytePointer+1] << 8) | image[bytePointer];
	      bytePointer += 2;
	      msw = image[bytePointer++];
	      break;
	  }
	  BDX_MDIO_WRITE(r1,0x204,msw);
	  BDX_MDIO_WRITE(r1,0x205,lsw);
	  BDX_MDIO_WRITE(r1,0x200,mailboxWrite);
  }
  
#ifdef AQ_VERBOSE
    printf ("Last DRAM data lsw=0x%x msw=0x%x \n",lsw, msw);
    msw = bdx_mdio_read(regs, r1, port, 0x202);
    lsw = bdx_mdio_read(regs, r1, port, 0x203);
    printf ("Last+1 DRAM address lsw=0x%x msw=0x%x \n",lsw, msw);
#endif  


  /*-- Clear any resets -----------------------------------*/
   BDX_MDIO_WRITE(r1,0,0);


  /*-- Release the uP --------------------------------------*/
    
    val = 1;    //upRunStall
    val |= 1<<6; //upRunStallOverride
    BDX_MDIO_WRITE(r1,0xC001,val);
    val |= 1<<15; //upReset
    BDX_MDIO_WRITE(r1,0xC001,val);
  
  /* Need to wait at least 100us. */
   msleep(200);

   BDX_MDIO_WRITE(r1,0xC001,0x40);
   for(i=100;i--;){
      msleep(20);
      val = bdx_mdio_read(regs, r1, port, 0xCC00);
      if(val&(1<<6)) break;
// ERR("1E.CC00.6 loop =%u\n",i);
   };
  /* if(!(val&(1<<6))){ // Reset timeout
       ERR("AQ2104 FW reset timeout 1E.CC00=%x\n",val);  
   };*/

   val = bdx_mdio_read(regs, r1, port, 0x20);
   val2 = bdx_mdio_read(regs, r1, port, 0xC885);
   val1 = val >> 8;
   val &= 0xFF;
   val2 &= 0xFF;
   ERR("AQ2104 FW ver: %x.%x.%x",val1,val,val2);  
//Enable AQRate speeds
    msleep(20);
	BDX_MDIO_WRITE(0x07,0x10,0x9101);
	BDX_MDIO_WRITE(0x07,0xC400,0x9C54);
	BDX_MDIO_WRITE(0x07,0x20,0x1001);
	// restart autoneg
        val=(1<<12)|(1<<9)|(1<<13);
	BDX_MDIO_WRITE(0x07,0x0,val);

#if 1  // led setup
/*
0xC430=f // Act led (right)  black-no link or no act; green-link blink on act
0xC431=80 // Speed led Amber -on on 10G 
Tehuti settings
0xC432=C040 // Speed led Green -on on 5G/2.5G/1G (black on 100M)

D-link settings
0xC432=C060 // Speed led Green -on on 5G/2.5/1G/100M
*/

// ERR("subsystem_vendor=%x subsystem_device=%x",sv,sd);

//ACT Led workaround - off on no link
    sv=priv->pdev->subsystem_vendor;
    sd=priv->pdev->subsystem_device;
    BDX_MDIO_WRITE(r1,0xC430,0x00);  
    BDX_MDIO_WRITE(r1,0xC431,0x80);  
//Link Led Configurtions
 if(((sv==0x1186)&&(sd==0x2900))){    //isDlink
    BDX_MDIO_WRITE(r1,0xC432,0xC060);  
  } 
  else
  {
    BDX_MDIO_WRITE(r1,0xC432,0xC040);  
  }


#endif

  
return rVal; 
} // AQ2104_mdio_reset

//-------------------------------------------------------------------------------------------------

static int  AQ2104_get_link_speed(struct bdx_priv *priv)
{
    unsigned short  val; //, leds;
    int             link = 0;
    u16		    ActLnk_MSK = 0;
    u16 	    sd, sv;
//DBG_ON;
    ENTER;
    val = bdx_mdio_read(priv->pBdxRegs, 7, priv->phy_mdio_port, 0xC810);
    val = bdx_mdio_read(priv->pBdxRegs, 7, priv->phy_mdio_port, 0xC810);
//DBG("AQ2104 7.C810 = 0x%x\n", val);
    val = val>>9;
    val &=0x1f;
    if (val == 4)                                         // Link up
    {
        val = bdx_mdio_read(priv->pBdxRegs, 7, priv->phy_mdio_port, 0xC800);
        val = bdx_mdio_read(priv->pBdxRegs, 7, priv->phy_mdio_port, 0xC800);
//DBG("AQ2104 7.c800 val=%x\n",val);
        val  &=  0xF;
	if((val >> 1)>0){
   	  sv=priv->pdev->subsystem_vendor;
   	  sd=priv->pdev->subsystem_device;
 	  if(!((sv==0x1186)&&(sd==0x2900))){    //isNotDlink
		ActLnk_MSK = 0xC0E0 ;
	  } 
          bdx_mdio_write(priv->pBdxRegs, 0x1E, priv->phy_mdio_port, 0xC430,0x0f | ActLnk_MSK);
	}
        switch(val >> 1)
        {
            case 5: //SPEED_RES_5GIG     
                link = SPEED_5000;
		DBG("AQ2104 5G link detected\n");
                break;

            case 4: //SPEED_RES_2.5GIG     
                link = SPEED_2500;
		DBG("AQ2104 2.5G link detected\n");
                break;

            case 3: //SPEED_RES_10GIG     
                link = SPEED_10000;
		DBG("AQ2104 10G link detected\n");
                break;

            case 2: // SPEED_RES_1GIG
                link = SPEED_1000;
                DBG("AQ2104 1G link detected\n");
                break;

            case 1: // SPEED_RES_100M
                link = SPEED_100;
                DBG("AQ2104 100M %s link detected\n", (val & 1 ) ? "" :"Half Duplex" );
                break;

            case 0: // SPEED_RES_10M
                link = 0;
        	bdx_mdio_write(priv->pBdxRegs, 0x1E, priv->phy_mdio_port, 0xC430,0x0);
                DBG("AQ2104 10M %s link detected. Not Supported\n", (val & 1 ) ? "" :"Half Duplex" );
                break;

        }
        priv->errmsg_count = 0;
    }
    else
    {
        bdx_mdio_write(priv->pBdxRegs, 0x1E, priv->phy_mdio_port, 0xC430,0x0);
    	if (++priv->errmsg_count < MAX_ERRMSGS)
    	{
    		DBG("AQ2104 link down.\n");
    	}
    }

//DBG_OFF;
    RET(link);

} // AQ2104_get_link_speed()

//-------------------------------------------------------------------------------------------------

u32 AQ2104_link_changed(struct bdx_priv *priv)
{
    u32 link, speed;

	speed = AQ2104_get_link_speed(priv);
//        DBG("AQ2104_link_changed speed=%u priv->link_speed=%u\n",speed,priv->link_speed);
	if(speed != priv->link_speed)
	{
		switch (speed)
		{
			case SPEED_10000:
				DBG("AQ2104 10G link detected\n");
				break;
			case SPEED_5000:
				DBG("AQ2104 5G link detected\n");
				break;
			case SPEED_2500:
				DBG("AQ2104 2.5G link detected\n");
				break;
			case SPEED_1000:
				DBG("AQ2104 1G link detected\n");
				break;
			case SPEED_100:
				DBG("AQ2104 100M link detected\n");
				break;
			case SPEED_10:
				DBG("AQ2104 10M link detected\n");
				break;
			default:
				DBG("AQ2104 link down.\n");
				break;
		}
		bdx_speed_changed(priv,speed);
	}
	link = 0;
	if((!speed) || (!(link =  (READ_REG(priv, regMAC_LNK_STAT) & MAC_LINK_STAT))))  // // XAUI link
	{
		u32 timeout;
		if (speed)
		{
			timeout = 1000000;      // 1/5 sec
			if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
			{
				bdx_speed_changed(priv,0);
				priv->link_loop_cnt = 0;
				DBG("AQ2104 trying to recover link after %d tries\n", LINK_LOOP_MAX);
			}
		}
		else
		{
			timeout = 5000000;      // 1 sec
		}
		DBG("AQ2104 link = 0x%x speed = 0x%x setting %d timer\n", link, speed, timeout);
		WRITE_REG(priv, 0x5150,timeout);
	}

	return speed;//link;

} // AQ2104_link_changed()

//-------------------------------------------------------------------------------------------------

#ifndef __TN40_OSX__

int AQ2104_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	
         ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full  | SUPPORTED_Autoneg  | SUPPORTED_TP | SUPPORTED_Pause);
        if(!priv->advertising) {
		priv->advertising   = ecmd->supported;
		priv->autoneg     = AUTONEG_ENABLE;
	}
    	ecmd->advertising   = priv->advertising;
#if 1
        ethtool_cmd_speed_set(ecmd,priv->link_speed);
#else
	ecmd->speed       =(__u16)  priv->link_speed;
	ecmd->speed_hi       =(__u16)  (priv->link_speed>>16);
#endif
	ecmd->duplex      = DUPLEX_FULL;
	ecmd->port        = PORT_TP;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg     = priv->autoneg;
#if defined(ETH_TP_MDI_AUTO)
	ecmd->eth_tp_mdix = ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	ecmd->eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	return 0;

} // AQ2104_get_settings()

#else

//-------------------------------------------------------------------------------------------------

int AQ2104_get_settings()
{

	return 0;

} // CX4_get_settings()

#endif

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

void AQ2104_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
        u16 dev=0x1e, led=0xC430, led_off=0, led_on=(1<<8);

	switch (op)
	{
		case PHY_LEDS_SAVE:
			priv->phy_ops.leds = bdx_mdio_read(priv->pBdxRegs, dev, priv->phy_mdio_port, led);
			bdx_mdio_write(priv->pBdxRegs, dev, priv->phy_mdio_port, led, led_off);
			break;

		case PHY_LEDS_RESTORE:
			bdx_mdio_write(priv->pBdxRegs, dev, priv->phy_mdio_port, led, priv->phy_ops.leds);

			break;

		case PHY_LEDS_ON:
			bdx_mdio_write(priv->pBdxRegs, dev, priv->phy_mdio_port, led, led_on);

			break;

		case PHY_LEDS_OFF:
			bdx_mdio_write(priv->pBdxRegs, dev, priv->phy_mdio_port, led, led_off);

			break;

		default:
			DBG("AQ2104_leds() unknown op 0x%x\n", op);
			break;

	}

} // AQ2104_leds()


int AQ2104_set_settings(struct net_device *netdev,
			      struct ethtool_cmd *ecmd)
{
        struct bdx_priv *priv = netdev_priv(netdev);
        void __iomem 	*regs=priv->pBdxRegs;
	u16 val, port=priv->phy_mdio_port;
	s32 speed=ethtool_cmd_speed(ecmd);

#if 0 // debug
		ERR("AQ2104 ecmd->cmd=%x\n", ecmd->cmd);
		ERR("AQ2104 speed=%u\n",speed);
		ERR("AQ2104 ecmd->autoneg=%u\n",ecmd->autoneg);
#endif
 
	if(AUTONEG_ENABLE == ecmd->autoneg){
//	ERR("AQ2104 speed 10G/1G/100m Autoneg\n");
	BDX_MDIO_WRITE(0x07,0x10,0x9101);
	BDX_MDIO_WRITE(0x07,0xC400,0x9C54);
	BDX_MDIO_WRITE(0x07,0x20,0x1001);
	priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | ADVERTISED_Pause);
	priv->autoneg     = AUTONEG_ENABLE;
	} else {
	priv->autoneg     = AUTONEG_DISABLE;
	switch(speed){
        case 10000: //10G
//		ERR("AQ2104 speed 10G\n");
		BDX_MDIO_WRITE(0x07,0x10,0x9001);
		BDX_MDIO_WRITE(0x07,0xC400,0x40);
		BDX_MDIO_WRITE(0x07,0x20,0x1001);
		priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
        	break;
        case 5000: //5G
//		ERR("AQ2104 speed 5G\n");
		BDX_MDIO_WRITE(0x07,0x10,0x9001);
		BDX_MDIO_WRITE(0x07,0xC400,0x840);
		BDX_MDIO_WRITE(0x07,0x20,0x1);
		priv->advertising = (/*ADVERTISED_5000baseT_Full | */ADVERTISED_Pause);
        	break;
        case 2500: //2.5G
//		ERR("AQ2104 speed 10G\n");
		BDX_MDIO_WRITE(0x07,0x10,0x9001);
		BDX_MDIO_WRITE(0x07,0xC400,0x440);
		BDX_MDIO_WRITE(0x07,0x20,0x1);
		priv->advertising = (/*ADVERTISED_2500baseT_Full |*/ ADVERTISED_Pause);
        	break;
        case 1000:  //1G
//		ERR("AQ2104 speed 1G\n");
		BDX_MDIO_WRITE(0x07,0x10,0x8001);
		BDX_MDIO_WRITE(0x07,0xC400,0x8040);
		BDX_MDIO_WRITE(0x07,0x20,0x1);
		priv->advertising = (ADVERTISED_1000baseT_Full | ADVERTISED_Pause);
        	break;
        case 100:   //100m
//		ERR("AQ2104 speed 100m\n");
		BDX_MDIO_WRITE(0x07,0x10,0x101);
		BDX_MDIO_WRITE(0x07,0xC400,0x40);
		BDX_MDIO_WRITE(0x07,0x20,0x1);
		priv->advertising = (ADVERTISED_100baseT_Full | ADVERTISED_Pause);
        	break;
        default :
		DBG("AQ2104 strange speed %u\n", speed);
             return -EINVAL;
        }
	}
// restart autoneg
        val=(1<<12)|(1<<9)|(1<<13);
	BDX_MDIO_WRITE(0x07,0x0,val);

	return 0;
}

//-------------------------------------------------------------------------------------------------

__init enum PHY_TYPE AQ2104_register(struct bdx_priv *priv)
{
    priv->isr_mask= IR_RX_FREE_0 | IR_LNKCHG0 |IR_LNKCHG1 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = AQ2104_mdio_reset;
    priv->phy_ops.link_changed = AQ2104_link_changed;
    priv->phy_ops.get_settings = AQ2104_get_settings;
    priv->phy_ops.ledset       = AQ2104_leds;
    priv->phy_ops.set_settings = AQ2104_set_settings;
    return PHY_TYPE_AQ2104;

} // AQ2104_init()

//-------------------------------------------------------------------------------------------------

