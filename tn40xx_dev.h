#ifndef _TN40XX_DEV_H
#define _TN40XX_DEV_H

#define BDX_DRV_VERSION   	"0.3.6.12.4"
#define DRIVER_AUTHOR     	"Tehuti networks"
#define BDX_DRV_DESC      	"Tehuti Network Driver"
#define BDX_DRV_NAME      	"tn40xx"
#define BDX_NIC_NAME      	"tn40xx"

// Supported PHYs:

#define PHY_MV88X3120
#define PHY_QT2025
#define PHY_TLK10232
#define PHY_AQ2104
#include "tn40.h"

unsigned short MV88X3120_register(struct bdx_priv *priv);
unsigned short TN8020_register(struct bdx_priv *priv);
unsigned short QT2025_register(struct bdx_priv *priv);
unsigned short CX4_register(struct bdx_priv *priv);
unsigned short TLK10232_register(struct bdx_priv *priv);
unsigned short AQ2104_register(struct bdx_priv *priv);

#define LDEV(_vid,_pid,_subdev,_msi,_ports,_phya,_phyb,_name)  \
    {_vid,_pid,_subdev,_msi,_ports,PHY_TYPE_##_phya,PHY_TYPE_##_phyb}

static struct bdx_device_descr  bdx_dev_tbl[] =
{
	LDEV(TEHUTI_VID,0x4010,0x4010,1,1,CX4,NA,           "TN4010 Clean SROM"),
	LDEV(TEHUTI_VID,0x4022,0x3015,1,1,QT2025,NA,        "TN9310 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4022,0x4d00,1,1,QT2025,NA,        "D-Link DXE-810S 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4022,0x8709,1,1,QT2025,NA,        "ASUS XG-C100F 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4022,0x8103,1,1,QT2025,NA,    "Edimax EN-8102PF 10GbE SPF+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4024,0x3015,1,1,MV88X3120,NA, "TN9210 10GBase-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4026,0x3015,1,1,TLK10232,NA,  "TN9610 10GbE SFP+ Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4025,0x2900,1,1,AQ2104,NA,    "D-Link DXE-810T 10GBase-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4025,0x3015,1,1,AQ2104,NA,        "TN9510 10GBase-T/NBASE-T Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4025,0x8102,1,1,AQ2104,NA,    "Edimax EN-8102P 10GbE Ethernet Adapter"),
	LDEV(TEHUTI_VID,0x4025,0x7203,1,1,AQ2104,NA,    "Promise Technology SANLink3 NBase-T1"),
    {0}
};

static struct pci_device_id bdx_pci_tbl[] =
{
    {TEHUTI_VID, 0x4010, TEHUTI_VID, 0x4010, 0, 0, 0},
    {TEHUTI_VID, 0x4022, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4022, DLINK_VID,  0x4d00, 0, 0, 0},
    {TEHUTI_VID, 0x4022, ASUS_VID,   0x8709, 0, 0, 0},
    {TEHUTI_VID, 0x4022, EDIMAX_VID, 0x8103, 0, 0, 0},
    {TEHUTI_VID, 0x4024, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4026, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4025, DLINK_VID,  0x2900, 0, 0, 0},
    {TEHUTI_VID, 0x4025, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4025, EDIMAX_VID, 0x8102, 0, 0, 0},
    {TEHUTI_VID, 0x4022, PROMISE_VID,0x7203, 0, 0, 0},
    {0}
};

#endif // _TN40XX_DEV_H
