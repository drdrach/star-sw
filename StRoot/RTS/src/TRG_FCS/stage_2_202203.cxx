#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "fcs_trg_base.h"
#include "fcs_ecal_epd_mask.h"

// Processing on the North or South DEP/IO flavoured board. 
// Inputs are up to 32 links but I already organized them according to strawman.
// output is 1 link over the external OUT connector.
// There are 20 inputs from ECAL, 8 from HCAL and 6 from PRE.
// This version combines HT trigger and physics triggers as Christian did in VHDL.

namespace{

  //version2 with top2 & bottom2 rows in trigger, missing far side column
  static const int EtoHmap[15][9][2] = {
    { { 0, 0},{ 0, 1},{ 0, 1},{ 0, 2},{ 0, 2},{ 0, 3},{ 0, 4},{ 0, 4},{ 0, 4}},
    { { 0, 0},{ 0, 1},{ 0, 1},{ 0, 2},{ 0, 2},{ 0, 3},{ 0, 4},{ 0, 4},{ 0, 4}},
    { { 1, 0},{ 1, 1},{ 1, 1},{ 1, 2},{ 1, 2},{ 1, 3},{ 1, 4},{ 1, 4},{ 1, 4}},
    { { 2, 0},{ 2, 1},{ 2, 1},{ 2, 2},{ 2, 2},{ 2, 3},{ 2, 4},{ 2, 4},{ 2, 4}},
    { { 2, 0},{ 2, 1},{ 2, 1},{ 2, 2},{ 2, 2},{ 2, 3},{ 2, 4},{ 2, 4},{ 2, 4}},
    { { 3, 0},{ 3, 1},{ 3, 1},{ 3, 2},{ 3, 2},{ 3, 3},{ 3, 4},{ 3, 4},{ 3, 4}},
    { { 3, 0},{ 3, 1},{ 3, 1},{ 3, 2},{ 3, 2},{ 3, 3},{ 3, 4},{ 3, 4},{ 3, 4}},
    { { 4, 0},{ 4, 1},{ 4, 1},{ 4, 2},{ 4, 2},{ 4, 3},{ 4, 4},{ 4, 4},{ 4, 4}},
    { { 5, 0},{ 5, 1},{ 5, 1},{ 5, 2},{ 5, 2},{ 5, 3},{ 5, 4},{ 5, 4},{ 5, 4}},
    { { 5, 0},{ 5, 1},{ 5, 1},{ 5, 2},{ 5, 2},{ 5, 3},{ 5, 4},{ 5, 4},{ 5, 4}},
    { { 6, 0},{ 6, 1},{ 6, 1},{ 6, 2},{ 6, 2},{ 6, 3},{ 6, 4},{ 6, 4},{ 6, 4}},
    { { 6, 0},{ 6, 1},{ 6, 1},{ 6, 2},{ 6, 2},{ 6, 3},{ 6, 4},{ 6, 4},{ 6, 4}},
    { { 7, 0},{ 7, 1},{ 7, 1},{ 7, 2},{ 7, 2},{ 7, 3},{ 7, 4},{ 7, 4},{ 7, 4}},
    { { 8, 0},{ 8, 1},{ 8, 1},{ 8, 2},{ 8, 2},{ 8, 3},{ 8, 4},{ 8, 4},{ 8, 4}},
    { { 8, 0},{ 8, 1},{ 8, 1},{ 8, 2},{ 8, 2},{ 8, 3},{ 8, 4},{ 8, 4},{ 8, 4}}
  } ;
}

void  fcs_trg_base::stage_2_202203(link_t ecal[], link_t hcal[], link_t pres[], geom_t geo, link_t output[])
{    
    int ns=geo.ns;
    if(fcs_trgDebug>=2) printf("Stage2v3 ns=%d\n",ns);

    // NS Marker
//    u_short params ;
//  if(ns==0) params = 0xBBAA ;
//    else params = 0xDDCC ;

    // Creating 2x2 row/column address map when called first time
    static u_int ETbTdep[16][10]; //DEP#
    static u_int ETbTadr[16][10]; //Input Link data address
    static u_int HTbTdep[10][6];  //DEP#
    static u_int HTbTadr[10][6];  //Input Link data address          
    static int first=0;
    if(first==0){
	first=1;
	//making map of 2x2 Ecal Sums of [4][4]
        for(int r=0; r<16; r++){
            printf("Ecal r=%2d : ",r);
            for(int c=0; c<10; c++){
                ETbTdep[r][c]= c/2 + (r/4)*5;
                ETbTadr[r][c]= c%2 + (r%4)*2;
                printf("%2d-%1d ",ETbTdep[r][c],ETbTadr[r][c]);
            }
            printf("\n");
        }
        //making map of 2x2 Hcal sums of [10][6]
        for(int r=0; r<10; r++){
            printf("HCal r=%2d : ",r);
            for(int c=0; c<6; c++){
                if      (r==0){
                    HTbTdep[r][c]= 6;
                    HTbTadr[r][c]= c;
                }else if(r==9){
                    HTbTdep[r][c]= 7;
                    HTbTadr[r][c]= c;
                }else{
                    HTbTdep[r][c]= c/2 + ((r-1)/4)*3;
                    HTbTadr[r][c]= c%2 + ((r-1)%4)*2;
                }
                printf("%2d-%1d ",HTbTdep[r][c],HTbTadr[r][c]);
            }
            printf("\n");
	}
    }
    
    // Ecal 2x2 "HT" trigger
    int ecal_ht = 0 ;
    for(int i=0;i<DEP_ECAL_TRG_COU;i++) {// 0..19
	for(int j=0;j<8;j++) {
	    if(s2_ch_mask[geo.ns] & (1ll<<i)) {
		ecal[i].d[j] = 0 ;
		continue ;
	    }
	    if(ecal[i].d[j] > EHTTHR) ecal_ht |= 1 ;
	}
    }
    
    // Hcal 2x2 "HT" trigger
    int hcal_ht = 0 ;
    for(int i=0;i<DEP_HCAL_TRG_COU;i++) {// 20..27
	for(int j=0;j<8;j++) {
	    if(s2_ch_mask[geo.ns] & (1ll<<(20+i))) {
		hcal[i].d[j] = 0 ;
		continue ;
	    }
	    if(hcal[i].d[j] > HHTTHR) hcal_ht |= 1 ;
	}
    }
    
    // Pres OR trigger
    int fpre_or = 0 ;
    for(int i=0;i<DEP_PRE_TRG_COU;i++) {// 28..32
	for(int j=0;j<8;j++) {
	    if(s2_ch_mask[geo.ns] & (1ll<<(28+i))) {
		pres[i].d[j] = 0 ;
		continue ;
	    }
//	    if(pres[i].d[j] > PHTTHR) fpre_or |= 1 ;	// Tonko: the FPRE S2 inputs are bitmasks so we _do_not_ apply PHTTHR here
	    if(pres[i].d[j]) fpre_or |= 1 ;
	}
    }
    
    //mapped Ecal 2x2
    for(int r=0; r<16; r++){
      for(int c=0; c<10; c++){
	e2x2[ns][r][c]=ecal[ETbTdep[r][c]].d[ETbTadr[r][c]];
      }
    }
    //mapped Hcal 2x2
    for(int r=0; r<10; r++){
      for(int c=0; c<6; c++){
	h2x2[ns][r][c]=hcal[HTbTdep[r][c]].d[HTbTadr[r][c]];
      }
    }

    //compute overlapping Hcal 4x4 sum of [9][5]
    //u_int hsum[9][5];
    for(int r=0; r<9; r++){
        if(fcs_trgDebug>=2) printf("H4x4 ");
        for(int c=0; c<5; c++){
            hsum[ns][r][c]
                = hcal[HTbTdep[r  ][c  ]].d[HTbTadr[r  ][c  ]]
                + hcal[HTbTdep[r  ][c+1]].d[HTbTadr[r  ][c+1]]
                + hcal[HTbTdep[r+1][c  ]].d[HTbTadr[r+1][c  ]]
                + hcal[HTbTdep[r+1][c+1]].d[HTbTadr[r+1][c+1]];
            //if(hsum[r][c] > 0xff) hsum[r][c]=0xff;  //Tonko says no point to saturate at 8bit here
            if(fcs_trgDebug>=2) printf("%5d ",hsum[ns][r][c]);
        }
        if(fcs_trgDebug>=2) printf("\n");
    }
    
    //PRES for QA
    if(fcs_trgDebug>0){
	for(int dep=0; dep<6; dep++) {
	    for(int j=0; j<4; j++) {
		for(int k=0; k<8; k++){
		    phit[ns][dep][j*8+k] = (pres[dep].d[j] >> k) & 0x1;
		}
	    }
	}
    }
    if(fcs_trgDebug>=2){
	for(int dep=0; dep<6; dep++) {
	    printf("PRES NS%1d DEP%1d : ",ns,dep);
	    for(int j=0; j<4; j++) {
		for(int k=0; k<8; k++){
		    phit[ns][dep][j*8+k] = (pres[dep].d[j] >> k) & 0x1;
		    printf("%1d", (pres[dep].d[j]>>k)&0x1);
		}
		printf(" ");
	    }
	    printf("\n");
	}
    }
    
    //compute overlapping Ecal 4x4 sums of [15][9]
    //take ratio with the closest hcal 4x4
    //u_int esum[15][9];
    //u_int sum[15][9];
    //float ratio[15][9];
    u_int EM1 =0, EM2 =0, EM3=0;
    u_int GAM1=0, GAM2=0, GAM3=0;
    u_int ELE1=0, ELE2=0, ELE3=0;
    u_int HAD1=0, HAD2=0, HAD3=0;
    u_int ETOT=0, HTOT=0;
    for(int r=0; r<15; r++){
        if(fcs_trgDebug>=2) printf("E4x4+H %2d  ",r);
        for(int c=0; c<9; c++){
            esum[ns][r][c]
                = ecal[ETbTdep[r  ][c  ]].d[ETbTadr[r  ][c  ]]
                + ecal[ETbTdep[r  ][c+1]].d[ETbTadr[r  ][c+1]]
                + ecal[ETbTdep[r+1][c  ]].d[ETbTadr[r+1][c  ]]
                + ecal[ETbTdep[r+1][c+1]].d[ETbTadr[r+1][c+1]];
            //if(esum[r][c] > 0xff) esum[r][c]=0xff; //Tonko says no point to saturate at 8bit here

	    // locate the closest hcal
            u_int h=hsum[ns][EtoHmap[r][c][0]][EtoHmap[r][c][1]];
                                 
	    // E+H sum
            sum[ns][r][c] = esum[ns][r][c] + h;
	    if(sum[ns][r][c]>0xff) sum[ns][r][c]=0xff;

            //in VHDL we will do esum>hsum*threshold. Ratio = H/(E+H) is for human only
            if(sum[ns][r][c]==0) {
	      ratio[ns][r][c]=0.0;
            }else{
	      ratio[ns][r][c] = float(esum[ns][r][c]) / float(esum[ns][r][c] + h);
            }

	    //check EPD hits using the mask
	    epdcoin[ns][r][c]=0;
	    for(int dep=0; dep<6; dep++){
		int mask;
		if(fcs_readPresMaskFromText==0){
		    mask = fcs_ecal_epd_mask[r][c][dep]; //from include file
		}else{
		    mask = PRES_MASK[r][c][dep]; //from static which was from text file 
		}
		for(int j=0; j<4; j++) {
		    for(int k=0; k<8; k++){
			if( (mask >> (j*8 + k)) & 0x1) { //if this is 0, don't even put the logic in VHDL
			    epdcoin[ns][r][c] |= (pres[dep].d[j] >> k) & 0x1;
			}
		    }
		}
	    }

	    // integer multiplication as in VHDL!
	    // ratio thresholds are in fixed point integer where 1.0==128
	    em[ns][r][c]=0;	
	    had[ns][r][c]=0;
	    u_int h128 = h*128 ;

// Tonko: matches newest Christian code
//	    if(h128 < esum[ns][r][c] * EM_HERATIO_THR){
	    if(h128 <= esum[ns][r][c] * EM_HERATIO_THR){
  	        em[ns][r][c]=1;
		if(sum[ns][r][c] > EMTHR1){
		    EM1 = 1;
		    if(epdcoin[ns][r][c]==0) {GAM1 = 1;} 
		    else                     {ELE1 = 1;}
		}
		if(sum[ns][r][c] > EMTHR2){
		    EM2 = 1;		
		    if(epdcoin[ns][r][c]==0) {GAM2 = 1;} 
		    else                     {ELE2 = 1;}
		}
		if(sum[ns][r][c] > EMTHR3){
		    EM3 = 1;		
		    if(epdcoin[ns][r][c]==0) {GAM3 = 1;} 
		    else                     {ELE3 = 1;}
		}
	    }
	    if(h128 > esum[ns][r][c] * HAD_HERATIO_THR){
  	        had[ns][r][c]=1;
		if(sum[ns][r][c] > HADTHR1) HAD1 = 1;
		if(sum[ns][r][c] > HADTHR2) HAD2 = 1;
		if(sum[ns][r][c] > HADTHR3) HAD3 = 1;
	    }
	    if(fcs_trgDebug>=2) printf("%5d %1d %3.2f ",sum[ns][r][c],epdcoin[ns][r][c],ratio[ns][r][c]);
	}
	if(fcs_trgDebug>=2) printf("\n");
    }
    
    //Ecal sub-crate sum
    u_int esub[4];
    esub[0] = esum[ns][ 0][0]+esum[ns][ 0][2]+esum[ns][ 0][4]+esum[ns][ 0][6]+esum[ns][ 0][8]
	    + esum[ns][ 2][0]+esum[ns][ 2][2]+esum[ns][ 2][4]+esum[ns][ 2][6]+esum[ns][ 2][8];
    esub[1] = esum[ns][ 4][0]+esum[ns][ 4][2]+esum[ns][ 4][4]+esum[ns][ 4][6]+esum[ns][ 4][8]
	    + esum[ns][ 6][0]+esum[ns][ 6][2]+esum[ns][ 6][4]+esum[ns][ 6][6]+esum[ns][ 6][8];
    esub[2] = esum[ns][ 8][0]+esum[ns][ 8][2]+esum[ns][ 8][4]+esum[ns][ 8][6]+esum[ns][ 8][8]
	    + esum[ns][10][0]+esum[ns][10][2]+esum[ns][10][4]+esum[ns][10][6]+esum[ns][10][8];
    esub[3] = esum[ns][12][0]+esum[ns][12][2]+esum[ns][12][4]+esum[ns][12][6]+esum[ns][12][8]
	    + esum[ns][14][0]+esum[ns][14][2]+esum[ns][14][4]+esum[ns][14][6]+esum[ns][14][8];
    for(int i=0; i<4; i++) if(esub[i]>0xff) esub[i]=0xff;
    
    //Hcal sub-crate sum
    u_int hsub[4];
    hsub[0] = hsum[ns][ 1][0]+hsum[ns][ 1][2]+hsum[ns][ 1][4];
    hsub[1] = hsum[ns][ 3][0]+hsum[ns][ 3][2]+hsum[ns][ 3][4];
    hsub[2] = hsum[ns][ 5][0]+hsum[ns][ 5][2]+hsum[ns][ 5][4];
    hsub[3] = hsum[ns][ 7][0]+hsum[ns][ 7][2]+hsum[ns][ 7][4];
    for(int i=0; i<4; i++) if(hsub[i]>0xff) hsub[i]=0xff;

    //Jet sum
    //u_int jet[3];
    u_int JP1=0,JP2=0;
    jet[ns][0] = esub[0] + esub[1] + hsub[0] + hsub[1];
    jet[ns][1] = esub[1] + esub[2] + hsub[1] + hsub[2];
    jet[ns][2] = esub[2] + esub[3] + hsub[2] + hsub[3];
    for(int i=0; i<3; i++){
      //if(jet[ns][i]>0xff) jet[ns][i]=0xff;
        if(jet[ns][i]>JETTHR1) JP1 = 1;
        if(jet[ns][i]>JETTHR2) JP2 = 1;
    }
    if(fcs_trgDebug>=2) printf("Jet = %3d %3d %3d\n",jet[ns][0],jet[ns][1],jet[ns][2]);

    //total ET
    etot[ns] = esub[0] + esub[1] + esub[2] + esub[3];
    htot[ns] = hsub[0] + hsub[1] + hsub[2] + hsub[3];
    //if(etot[ns]>0xff) etot[ns]=0xff;
    //if(htot[ns]>0xff) htot[ns]=0xff;
    if(etot[ns]>ETOTTHR) ETOT=1;
    if(htot[ns]>HTOTTHR) HTOT=1;
    if(fcs_trgDebug>=2) printf("E/H Tot = %3d %3d\n",etot[ns],htot[ns]);

    //sending output bits lower 8 bits
    //output[0].d[0] = EM1  + (EM2 <<1) + (EM3 <<2) + 0x80; // Tonko: added last bit
    output[0].d[0] = EM1  + (EM2 <<1) + (EM3 <<2);
    output[0].d[1] = ELE1 + (ELE2<<1) + (ELE3<<2);
    output[0].d[2] = GAM1 + (GAM2<<1) + (GAM3<<2);
    output[0].d[3] = HAD1 + (HAD2<<1) + (HAD3<<2);
    output[0].d[4] = JP1  + (JP2 <<1);
    output[0].d[5] = ETOT + (HTOT<<1);
    output[0].d[6] = (fpre_or<<2) | (hcal_ht<<1) | (ecal_ht); //HT
//    output[0].d[7] = 0xCD ;   
    output[0].d[7] = 0 ;   

    // upper 8 bits
    output[1].d[0] = 0 ;
    output[1].d[1] = 0 ;
    output[1].d[2] = 0 ;
    output[1].d[3] = 0 ;
    output[1].d[4] = 0 ;
    output[1].d[5] = 0 ;
    output[1].d[6] = 0 ;
//    output[1].d[7] = 0xAB ;
    output[1].d[7] = 0 ;

    if(fcs_trgDebug>=1){    
      printf("STG2 NS%1d = %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
	     ns,
	     output[1].d[0],output[0].d[0],output[1].d[1],output[0].d[1],
	     output[1].d[2],output[0].d[2],output[1].d[3],output[0].d[3],
	     output[1].d[4],output[0].d[4],output[1].d[5],output[0].d[5],
	     output[1].d[6],output[0].d[6],output[1].d[7],output[0].d[7]);
    }

    return ;
}

