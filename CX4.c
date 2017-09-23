#include "tn40.h"

#define LINK_LOOP_MAX   (10)

void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//-------------------------------------------------------------------------------------------------

int CX4_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{

    return 0;

} // CX4_mdio_reset

//-------------------------------------------------------------------------------------------------

u32 CX4_link_changed(struct bdx_priv *priv)
{
	u32 link=0;

	if (priv->link_speed != SPEED_10000)
	{
		bdx_speed_changed(priv, SPEED_10000);
	}
	link = READ_REG(priv, regMAC_LNK_STAT) & MAC_LINK_STAT;
	if (link)
	{
		link = SPEED_10000;
		DBG("CX4 link speed is 10G\n");
	}
	else
    {
		if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
		{
			DBG("CX4 MAC reset\n");
			priv->link_speed    = 0;
			priv->link_loop_cnt = 0;
		}
		DBG("CX4 no link, setting 1/5 sec timer\n");
		WRITE_REG(priv, 0x5150,1000000); // 1/5 sec timeout
    }

	return link;

} // CX4_link_changed()

//-------------------------------------------------------------------------------------------------

#ifndef __TN40_OSX__

int CX4_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_AUI | SUPPORTED_Pause);
	ecmd->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
	ecmd->speed       = SPEED_10000;
	ecmd->duplex      = DUPLEX_FULL;
	ecmd->port        = PORT_AUI;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg     = AUTONEG_DISABLE;

	return 0;

} // CX4_get_settings()

#else

//-------------------------------------------------------------------------------------------------

int CX4_get_settings()
{

	return 0;

} // CX4_get_settings()

#endif

//-------------------------------------------------------------------------------------------------

void CX4_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
	switch (op)
	{
		case PHY_LEDS_SAVE:
			break;

		case PHY_LEDS_RESTORE:
			break;

		case PHY_LEDS_ON:
			break;

		case PHY_LEDS_OFF:
			break;

		default:
			DBG("CX4_leds() unknown op 0x%x\n", op);
			break;

	}

} // CX4_leds()


//-------------------------------------------------------------------------------------------------

enum PHY_TYPE CX4_register(struct bdx_priv *priv)
{
	priv->isr_mask= IR_RX_FREE_0 | IR_LNKCHG0 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = CX4_mdio_reset;
    priv->phy_ops.link_changed = CX4_link_changed;
    priv->phy_ops.get_settings = CX4_get_settings;
    priv->phy_ops.ledset       = CX4_leds;

    return PHY_TYPE_CX4;

} // CX4_init()

//-------------------------------------------------------------------------------------------------

