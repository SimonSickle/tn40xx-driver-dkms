#include "tn40.h"
#include "QT2025_phy.h"

#define LINK_LOOP_MAX   (10)

void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//-------------------------------------------------------------------------------------------------

__init int QT2025_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{
    u16 *phy_fw= QT2025_phy_firmware,j, fwVer01, fwVer2, fwVer3, module;
    int s_fw, i, a;
    int phy_id= 0, rev= 0;
    void __iomem 	*regs=priv->pBdxRegs;

    port=bdx_mdio_look_for_phy(regs,port);
    if (port<0) return 0; // PHY not found

    phy_id= bdx_mdio_read(regs, 1, port, 0xD001);
    //  ERR("PHY_ID %x, port=%x\n", phy_id, port);

    switch(0xFF&(phy_id>> 8))
    {
        case 0xb3:
            rev= 0xd;
            s_fw= sizeof(QT2025_phy_firmware) / sizeof(u16);
            break;
        default:
            ERR("PHY ID =0x%x, returning\n", (0xFF & (phy_id >> 8)));
            return 1;
    }

    switch(rev)
    {
        default:
            ERR("bdx: failed unknown PHY ID %x\n", phy_id);
            \
            return 1;
        case 0xD:
            BDX_MDIO_WRITE(1,0xC300,0x0000);
            BDX_MDIO_WRITE(1,0xC302,0x4);
            BDX_MDIO_WRITE(1,0xC319,0x0038);
            //         BDX_MDIO_WRITE(1,0xC319,0x0088);  //1G
            BDX_MDIO_WRITE(1,0xC31A,0x0098);
            BDX_MDIO_WRITE(3,0x0026,0x0E00);
            //         BDX_MDIO_WRITE(3,0x0027,0x08983);  //10G
            BDX_MDIO_WRITE(3,0x0027,0x0893);  //10G
            //         BDX_MDIO_WRITE(3,0x0027,0x01092);    //1G     //3092
            BDX_MDIO_WRITE(3,0x0028,0xA528);
            BDX_MDIO_WRITE(3,0x0029,0x03);
            BDX_MDIO_WRITE(1,0xC30A,0x06E1);
            BDX_MDIO_WRITE(1,0xC300,0x0002);
            BDX_MDIO_WRITE(3,0xE854,0x00C0);

            /* Dump firmware starting at address 3.8000 */
            for (i = 0, j = 0x8000, a = 3; i < s_fw; i++, j++)
            {
                if(i == 0x4000)
                {
                    a = 4;
                    j = 0x8000;
                }
                if (phy_fw[i] < 0x100)
                    BDX_MDIO_WRITE(a, j, phy_fw[i]);
            }
            BDX_MDIO_WRITE(3, 0xE854, 0x0040);
            for(i = 60; i; i--)
            {
                msleep(50);
                j = bdx_mdio_read(regs, 3, port, 0xD7FD);
                if(!(j== 0x10 || j== 0))
                    break;
            }
            if(!i)  ERR("PHY init error\n");
            break;
    }
    fwVer01 = bdx_mdio_read(regs, 3, port, 0xD7F3);
    fwVer2  = bdx_mdio_read(regs, 3, port, 0xD7F4);
    fwVer3  = bdx_mdio_read(regs, 3, port, 0xD7F5);
    module  = bdx_mdio_read(regs, 3, port, 0xD70C);

    printk(KERN_INFO "%s: QT2025 FW version %d.%d.%d.%d module type 0x%x",
    		priv->drv_name,
    		((fwVer01 >> 4) & 0xf),
    		(fwVer01 & 0xf),
    		(fwVer2 & 0xff),
    		(fwVer3 & 0xff),
    		(u32)(module & 0xff));

    return 0;

} // QT2025_mdio_reset

/*
 * Module types:
 *
		1 = 10GBASE-LRM
		2 = 10GBASE-SR
		3 = 10GBASE-LR
		4 = SFP+ passive copper direct attach cable
		5 = 1000BASE-SX
		6 = 1000BASE-LX
		7 = Invalid Module Type
		8 = 1000BASE-CX (1GE copper cable)
		9 = 1000BASE-T
		10 (0xA) = 10GBASE-ER
		11 (0xB) = SFP+ active copper direct attach cable
		>11 reserved for future use
*/
//-------------------------------------------------------------------------------------------------

u32 QT2025_link_changed(struct bdx_priv *priv)
{
	 int	speed;
     u32 	link	= 0;
     u16 	module	= (bdx_mdio_read(priv->pBdxRegs, 3, priv->phy_mdio_port, 0xD70C)) & 0xFF;

     switch (module)
     {
		 case 5:
		 case 6:
		 case 8:
		 case 9:
			 speed = SPEED_1000;
			 break;

		 default:
			 speed = SPEED_10000;
			 break;

     }
     if (priv->link_speed != speed)
     {
    	 bdx_speed_changed(priv, speed);
     }
     link = READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT;
     if (link)
     {
    	 link = speed;
    	 ERR("QT2025 link speed is %s\n", (speed == SPEED_10000) ? "10G" : "1G");
     }
     else
     {
    	 if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
    	 {
    		 DBG("QT2025 MAC reset\n");
    		 priv->link_speed    = 0;
    		 priv->link_loop_cnt = 0;
    	 }
    	 DBG("QT2025 no link, setting 1/5 sec timer\n");
    	 WRITE_REG(priv, 0x5150,1000000); // 1/5 sec timeout
     }

     return link;

} // QT2025_link_changed()

//-------------------------------------------------------------------------------------------------

int QT2025_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
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

} // QT2025_get_settings()

//-------------------------------------------------------------------------------------------------

void QT2025_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
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
			DBG("QT2025_leds() unknown op 0x%x\n", op);
			break;

	}

} // QT2025_leds()

//-------------------------------------------------------------------------------------------------

__init unsigned short QT2025_register(struct bdx_priv *priv)
{
	priv->isr_mask= IR_RX_FREE_0 |  IR_LNKCHG0 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = QT2025_mdio_reset;
    priv->phy_ops.link_changed = QT2025_link_changed;
    priv->phy_ops.get_settings = QT2025_get_settings;
    priv->phy_ops.ledset       = QT2025_leds;

    return PHY_TYPE_QT2025;

} // QT2025_init()

//-------------------------------------------------------------------------------------------------

