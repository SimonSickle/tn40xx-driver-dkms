#include "tn40.h"
#include "TLK10232_phy.h"

#define LINK_LOOP_MAX   (10)

void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//-------------------------------------------------------------------------------------------------

__init int TLK10232_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{
	void __iomem 	*regs=priv->pBdxRegs;
	int 			regVal, j=0;
//DBG_ON;


	// Device reset
	BDX_MDIO_WRITE(0x1e, GLOBAL_CONTROL_1,  0x8610); 
	regVal = bdx_mdio_read(regs, 0x1e, port, GLOBAL_CONTROL_1);
	DBG("TLK10232_mdio_reset() GLOBAL_CONTROL_1 = 0x%x\n", regVal);
	msleep(100);

	// Device config
	BDX_MDIO_WRITE(0x7, AN_CONTROL, 	0x2000);
	BDX_MDIO_WRITE(0x1, LT_TRAIN_CONTROL,   0x0); 
	BDX_MDIO_WRITE(0x1e, HS_SERDES_CONTROL_3, 0x9518); 
	BDX_MDIO_WRITE(0x1e, HS_SERDES_CONTROL_4, 0x3300); 
	msleep(100);

	// Datapath reset
	do
	{
		BDX_MDIO_WRITE(0x1e, RESET_CONTROL,     	    0x8);
		BDX_MDIO_WRITE(0x1e, RESET_CONTROL,     	    0x0);
		msleep(300);

		regVal = bdx_mdio_read(regs, 0x1e, port, CHANNEL_STATUS_1);
		regVal = bdx_mdio_read(regs, 0x1e, port, CHANNEL_STATUS_1);
		DBG("TLK10232_mdio_reset() CHANNEL_STATUS_1 = 0x%x Fail:%x loops:%d (Fail mask 0x0100)\n", regVal, (regVal & 0x0100), j);
	} while ((regVal & 0x0100) && (j++ < 100));

	msleep(900);

//DBG_OFF;
    return 0;

} // TLK10232_mdio_reset


//-------------------------------------------------------------------------------------------------

u32 TLK10232_link_changed(struct bdx_priv *priv)
{
	 int	speed;
     u32 	link	= 0;
DBG_ON;
     speed = SPEED_10000;


	//Set GPIO[1] to output 0
    	 WRITE_REG(priv, 0x51E0,0x30010004); // GPIO_O WR CMD
    	 WRITE_REG(priv, 0x51F0,0x3FD);
    	 WRITE_REG(priv, 0x51E0,0x30010006); // GPIO_OE_ WR CMD
    	 WRITE_REG(priv, 0x51F0,0x3FD); // GPIO_OE_ DATA
    	/* WRITE_REG(priv, 0x51E0,0x30000006); // GPIO_OE_ RD CMD
	ERR("TLK10232 GPIO set to = 0x%x\n",READ_REG(priv, 0x51F0));*/

     if (priv->link_speed != speed)
     {
    	 bdx_speed_changed(priv, speed);
     }
     link = READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT;
     if (link)
     {
    	 link = speed;
    	 DBG("TLK10232 link speed is %s\n", (speed == SPEED_10000) ? "10G" : "1G");
     }
     else
     {
    	 if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
    	 {
    		 //DBG("TLK10232 MAC reset\n");
    		 priv->link_speed    = 0;
    		 priv->link_loop_cnt = 0;
    	 }
    	 //DBG("TLK10232 no link, setting 1/5 sec timer\n");
    	 WRITE_REG(priv, 0x5150,1000000); // 1/5 sec timeout
     }

     return link;
DBG_OFF;
} // TLK10232_link_changed()

//-------------------------------------------------------------------------------------------------

int TLK10232_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   	= ( SUPPORTED_10000baseT_Full | SUPPORTED_FIBRE | SUPPORTED_Pause);
	ecmd->advertising 	= (ADVERTISED_10000baseT_Full | ADVERTISED_Pause );
	if (READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT)
	{
		ecmd->speed     =  priv->link_speed;
	}
	else
	{
		ecmd->speed     = 0;
	}
	ecmd->duplex      	= DUPLEX_FULL;
	ecmd->port        	= PORT_FIBRE;
	ecmd->transceiver 	= XCVR_EXTERNAL;
	ecmd->autoneg     	= AUTONEG_DISABLE;

	return 0;

} // TLK10232_get_settings()

//-------------------------------------------------------------------------------------------------

void TLK10232_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
	switch (op)
	{
		case PHY_LEDS_SAVE:
			break;

		case PHY_LEDS_RESTORE:
			break;

		case PHY_LEDS_ON:
			WRITE_REG(priv, regBLNK_LED, 4);
			break;

		case PHY_LEDS_OFF:
			WRITE_REG(priv, regBLNK_LED, 0);
			break;

		default:
			DBG("TLK10232_leds() unknown op 0x%x\n", op);
			break;

	}

} // TLK10232_leds()

//-------------------------------------------------------------------------------------------------

__init unsigned short TLK10232_register(struct bdx_priv *priv)
{
	priv->isr_mask= IR_RX_FREE_0 |  IR_LNKCHG0 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = TLK10232_mdio_reset;
    priv->phy_ops.link_changed = TLK10232_link_changed;
    priv->phy_ops.get_settings = TLK10232_get_settings;
    priv->phy_ops.ledset       = TLK10232_leds;

    return PHY_TYPE_TLK10232;

} // TLK10232_init()

//-------------------------------------------------------------------------------------------------

