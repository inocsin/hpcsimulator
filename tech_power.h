
#ifndef _TECHNOLOGY_AREA_POWER_H
#define _TECHNOLOGY_AREA_POWER_H

/* This file contains parameters for 65nm, 45nm*/


/*======================Parameters for Leakage Power===========================*/
#if(PARM(TECH_POINT) == 65)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_leak_nW     (4.256)
#define INV_leak_nW     (3.709)
#define DFF_leak_nW     (19.595)
#define AOI_leak_nW     (6.975)
#define MUX2_leak_nW    (6.229 )


#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_leak_nW     (11.375)
#define INV_leak_nW     (11.318)
#define DFF_leak_nW     (53.712)
#define AOI_leak_nW     (17.159)
#define MUX2_leak_nW    ( 16.400)


#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_leak_nW     (25.334)
#define INV_leak_nW     (26.363)
#define DFF_leak_nW     (121.833)
#define AOI_leak_nW     (33.371)
#define MUX2_leak_nW    (36.226)

#endif

#elif(PARM(TECH_POINT) == 45)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_leak_nW     (3.253654)
#define INV_leak_nW     (2.658218)
#define DFF_leak_nW     (13.359276)
#define AOI_leak_nW     (6.253391)
#define MUX2_leak_nW    (4.528161)


#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_leak_nW     (9.264710)
#define INV_leak_nW     (8.462244)
#define DFF_leak_nW     (34.708040)
#define AOI_leak_nW     (17.188076)
#define MUX2_leak_nW    (12.075659)



#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_leak_nW     (20.125159)
#define INV_leak_nW     (19.306026)
#define DFF_leak_nW     (75.611883)
#define AOI_leak_nW     (36.134066)
#define MUX2_leak_nW    (22.682192)

#endif
#endif

/*======================Parameters for Load Capacitance===========================*/
#if(PARM(TECH_POINT) == 65)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_load_pF     (0.0012+0.0011)
#define INV_load_pF     (0.0012)
#define DFF_load_pF     (0.0008+0.0011)
#define AOI_load_pF     (0.0011+0.0012+0.0011+0.0011)
#define MUX2_load_pF    (0.0007 +0.0007+.0017)
#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_load_pF     (0.0012+ 0.0011)
#define INV_load_pF     (0.0012)
#define DFF_load_pF     (.0008+0.0011)
#define AOI_load_pF     (0.0012+0.0012+ 0.0011+0.0012)
#define MUX2_load_pF    (0.0007+0.0007+0.0017)
#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_load_pF     (0.0012+0.0012)
#define INV_load_pF     (0.0012)
#define DFF_load_pF     (0.0008+0.0011)
#define AOI_load_pF     (0.0012+0.0012+0.0012+0.0012+0.0012)
#define MUX2_load_pF    (0.0012+ 0.0009+ 0.0019)

#endif

#elif(PARM(TECH_POINT) == 45)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_load_pF     (0.0005915+0.0006139)
#define INV_load_pF     (0.0006537)
#define DFF_load_pF     (0.0004264+0.0005393)
#define AOI_load_pF     (0.0005761+0.0006128+0.0006082+0.0006109)
#define MUX2_load_pF    (0.0004026+0.0004506+0.0009332+0.0004883)
#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_load_pF     (0.000606+0.0006321)
#define INV_load_pF     (0.000682)
#define DFF_load_pF     (0.0004413+0.0005526)
#define AOI_load_pF     (0.000588+0.0006294+0.0006235+0.0006313)
#define MUX2_load_pF    (0.0004169+ 0.0004638+0.0009579)
#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_load_pF     (0.0006184+0.0006476)
#define INV_load_pF     (0.0006999)
#define DFF_load_pF     (0.0004516+0.0005616)
#define AOI_load_pF     (0.0005973+0.0006431+0.0006333+0.0006467)
#define MUX2_load_pF    (0.0004249+0.000473+0.0009765)

#endif
#endif

/*======================Parameters for Internal Energy===========================*/
#if(PARM(TECH_POINT) == 65)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_int_J     (0.00001+0.0002)
#define INV_int_J     (0.00005)  /*0.0000 in lib*/
#define DFF_int_J     (0.0028)
#define AOI_int_J     (0.0002+0.0002+0.0007+0.0007)
#define MUX2_int_J    (0.0012+0.0009+0.0018)
#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_int_J     (0.0001+0.0002)
#define INV_int_J     (0.00005)  /*0.0000 in lib*/
#define DFF_int_J     (0.0028)
#define AOI_int_J     (0.0002+.0001+0.0006+0.0006)
#define MUX2_int_J    (0.0011+0.0009+0.00175)
#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_int_J     (0.0001+0.0003)
#define INV_int_J     (0.0005)
#define DFF_int_J     (0.00305)
#define AOI_int_J     (0.00025+0.0002+0.0008+0.0008+0.00105)
#define MUX2_int_J    (0.00225+0.0018+0.0028 )
#endif

#elif(PARM(TECH_POINT) == 45)
#if(PARM(TRANSISTOR_TYPE) == HVT)
#define NOR_int_J     (0.0001331+0.0003787)
#define INV_int_J     (0.00002634)
#define DFF_int_J     (0.001585)
#define AOI_int_J     (0.0001556+0.001207+0.001275+0.001487)
#define MUX2_int_J    (0.0001022+0.0002146+0.0008544+0.001122)
#elif(PARM(TRANSISTOR_TYPE) == NVT)
#define NOR_int_J     (0.0001178+0.0003667)
#define INV_int_J     (0.00001899)
#define DFF_int_J     (0.001571)
#define AOI_int_J     (0.0001414+0.0001192+0.0005183+0.0005243)
#define MUX2_int_J    (0.0007351+0.000565+0.0006304)
#elif(PARM(TRANSISTOR_TYPE) == LVT)
#define NOR_int_J     (0.0001102+0.0003439)
#define INV_int_J     (0.000024675)
#define DFF_int_J     (0.001559)
#define AOI_int_J     (0.00013005+0.0001045+0.0004761+0.0004814)
#define MUX2_int_J    (0.0007219+0.0005528+0.0006113)
#endif
#endif


#endif /* _SIM_POWER_V2_H */
