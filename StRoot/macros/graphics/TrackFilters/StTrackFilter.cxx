#include "StTrackFilter.h"
#include "TTableSorter.h"
#include "TTableIter.h"
#include "tables/St_g2t_tpc_hit_Table.h"
#include "tables/St_g2t_svt_hit_Table.h"
#include "tables/St_tcl_tphit_Table.h"
#include "tables/St_tpt_track_Table.h"
#include "tables/St_dst_track_Table.h"
#include "tables/St_g2t_track_Table.h"
#include "tables/St_g2t_vertex_Table.h"
#include "StCL.h"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// StTrackFilter virtual base class                                     //
//                                                                      //
//                                                                      //
//  Submit any problem with this code via begin_html <A HREF="http://www.star.bnl.gov/STARAFS/comp/sofi/bugs/send-pr.html"><B><I>"STAR Problem Report Form"</I></B></A> end_html
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(StTrackFilter)

//_____________________________________________________________________________
Int_t StTrackFilter::SetTrack_P(Int_t *track_p,Int_t n)
{
  m_LTrackP = n;
  StCL::ucopy(track_p,m_Track_P, m_LTrackP);
  return 0; 
}
//_____________________________________________________________________________
Int_t StTrackFilter::SetTptID(Int_t *track_id,Int_t n)
{
   m_Ltpt_track = n;
   StCL::ucopy(track_id,m_tpt_track_id, m_Ltpt_track); 
   return 0;
}
//_____________________________________________________________________________
Int_t StTrackFilter::SetId_globtrk(Int_t *id_globtrk,Int_t n)
{
   m_Lid_globtrk = n;
   StCL::ucopy(id_globtrk,m_id_globtrk, m_Lid_globtrk); 
   return 0;
}
//_____________________________________________________________________________
Int_t StTrackFilter::SetGe_pid(Int_t pid){ m_Ge_pid  = pid; return m_Ge_pid;}

//_____________________________________________________________________________
Int_t StTrackFilter::Reset(Int_t reset)
{
 m_G2t_track = 0; m_G2t_vertex = 0;
 SafeDelete(m_NextG2tTrack);
 return reset; 
}
//_____________________________________________________________________________
Int_t StTrackFilter::SubChannel(St_tcl_tphit &hit, Int_t rowNumber,Size_t &size, Style_t &style)
{
  size = 0.5;
  return kGreen;
  return hit[rowNumber].id_globtrk;
  if ( m_Lid_globtrk)  { 
    for (int i =0; i <m_Lid_globtrk; i++) {
      if (hit[rowNumber].id_globtrk !=  m_id_globtrk[i]) continue;
      printf("hit %d %ld \n", rowNumber, hit[rowNumber].id_globtrk); 
      style = 6;
      size = 2;
      return kGreen+rowNumber%20;       
    }
  }
  return 0;
}
//_____________________________________________________________________________
Int_t StTrackFilter::SubChannel(St_g2t_svt_hit &hit, Int_t rowNumber,Size_t &size,Style_t &style)
{
  if ( m_LTrackP)  { 
    for (int i =0; i <m_LTrackP; i++) {
      if (hit[rowNumber].track_p !=  m_Track_P[i]) continue;
      printf("hit %d %ld \n", rowNumber, hit[rowNumber].track_p); 
      style = 4;
      size = 1;
      return kBlue; 
    }
  }
  return 0;
}
//_____________________________________________________________________________
Int_t StTrackFilter::SubChannel(St_g2t_tpc_hit &hit, Int_t rowNumber,Size_t &size,Style_t &style)
{
 Int_t color = 0;
 style = 4;
 size  = 1;
 long track_p = hit[rowNumber].track_p; //  Id of parent track 
 if (m_G2t_track) {

   TTableIter &nextTrack = *m_NextG2tTrack;
   St_g2t_track &track     = *((St_g2t_track*)m_G2t_track->GetTable());
   Int_t indxColor         =  nextTrack[0];
   
   if (indxColor >= 0 ) // Check whether this table does contain any "ge_pid" we are looking for
   {
     Int_t indxRow           = -1;

     color  = track[indxColor].ge_pid+1;

     while ( (indxRow = nextTrack()) >= 0 ) {
       if (track_p != track[indxRow].id) continue;

       if (m_G2t_vertex && track[indxRow].stop_vertex_p) {
          TTableIter nextVertex(m_G2t_vertex,track[indxRow].stop_vertex_p);

          if (nextVertex.GetNRows() > 0) {
             St_g2t_vertex &vertex = *((St_g2t_vertex*)m_G2t_vertex->GetTable());
             while ( (indxRow = nextVertex() ) >=0 ) {
               Float_t *x = &(vertex[indxRow].ge_x[0]);
               Float_t dist = x[0]*x[0]+x[1]*x[1];
               Bool_t condition = dist > 133*133 && dist < 179*179; 

                if ((vertex[indxRow].ge_proc == 5) && condition ) 
                        return color%8+1;                             
             }
 	   }
         }

         return 0;      
       }
    }
 }
  if ( m_LTrackP)  { 
    for (int i =0; i <m_LTrackP; i++) {
      if ( track_p !=  m_Track_P[i])  continue;
      printf("hit %d %ld  \n", rowNumber, track_p); 
      return kBlue; 
    }
  }
  return 0;
}

//_____________________________________________________________________________
Int_t StTrackFilter::Channel(const TTableSorter *tableObject,Int_t index,Size_t &size,Style_t &style)
{
  //
  // This is an example how one can separate tableObject's by the table "name"
  // rather by the table "type" as the next "Channel" does
  //
 Int_t rowNumber = tableObject->GetIndex(UInt_t(index));
 
 Color_t color = -1;
 TString mStr = tableObject->GetTable()->GetName();

 if( mStr == "g2t_track" && !m_G2t_track) {
   if (m_NextG2tTrack) delete m_NextG2tTrack;
   m_NextG2tTrack = new TTableIter(tableObject,m_Ge_pid);
   m_G2t_track = tableObject;
 }
 else if( mStr == "g2t_vertex" && !m_G2t_vertex) {
   m_G2t_vertex   = tableObject;   
 }
 else if( mStr == "g2t_tpc_hit") {
   St_g2t_tpc_hit &hit = *((St_g2t_tpc_hit *)tableObject->GetTable());    
   color = SubChannel(hit, rowNumber, size, style);
 }
 else if( mStr == "g2t_svt_hit") {
   St_g2t_svt_hit &hit = *((St_g2t_svt_hit *)tableObject->GetTable());     
   color = SubChannel(hit,rowNumber,size,style);
 }
 else if( mStr == "tphit" ) {
   St_tcl_tphit &hit = *((St_tcl_tphit *)tableObject->GetTable());     
   color = SubChannel(hit,rowNumber,size,style);
 }
 return color; 
}

//_____________________________________________________________________________
Int_t StTrackFilter::Channel(const TTable *tableObject,Int_t rowNumber,Size_t &size,Style_t &style)
{
  // Introduce in here your own rule to select a particular row of the given tableObjectvertex
  //
  // Input:
  //   St_Table *tableObject - pointer
  //   Int_t rowNumber       - the current row number
  // 
  // Return value: > 0 the color index this vertex will be drawn
  //               = 0 this vertex will be not drawn
  //               < 0 no track will be drawn fro  now untill StVirtualFilter::Reset called
  // Output:
  //         size (option) - the marker size to be used to draw this vertex
  //         style(option) - the marker style to be used to draw this vertex
  // Note: One may not assign any value for the output parameters size and style
  //       The "default" values will be used instead.
  //
  // This is an example how one can separate tableObject's by the table "type"
  // rather by the table "name" as the previous "Channel" does
  //
  static int colorIndex = 0;
  TString mStr = tableObject->GetType();

  if( mStr == "dst_track" ) {
    St_dst_track &track = *((St_dst_track *)tableObject); 
    return SubChannel(track, rowNumber, size, style);
  } else {  

   colorIndex++;
   if (colorIndex > 20) colorIndex = 0;
   return kBlue+colorIndex;

   St_tpt_track &track = *((St_tpt_track *)tableObject); 
   if ( m_Ltpt_track )  { 
     for (int i =0; i < m_Ltpt_track; i++) {
        if (track[rowNumber].id != m_tpt_track_id[i]) continue;
        printf(" track %d %ld  \n", rowNumber,track[rowNumber].id); 
        style = 6;
        size = 2;
        //       return kGreen+rowNumber%20; 
        colorIndex++;
        if (colorIndex > 20) colorIndex = 0;
        // return kBlue+colorIndex;
        return kGreen;
     }
   }
 }
 return 0; 
}
//_____________________________________________________________________________
Int_t StTrackFilter::SubChannel(St_dst_track   &track, Int_t rowNumber,Size_t &size,Style_t &style)
{
  // SubChannel to provide a selections for St_dst_track tracks.
  //
  // 1. by curvature - all track with curvature > 0.01 are red
  // 2. by track charge (if m_Charge_Sign != 0 ). 
  //    The sung of m_Charge_Sign defines which track has to be selected.
  //
  static int colorIndex = 0;
  static Float_t minCurv = track[rowNumber].curvature;
  static Float_t maxCurv = track[rowNumber].curvature;
         Float_t curCurv = track[rowNumber].curvature;
  colorIndex++;
  if (curCurv > maxCurv) {maxCurv = curCurv; printf(" MaxCurv = %f\n", curCurv); }
  if (curCurv < minCurv) {minCurv = curCurv; printf(" MinCurv = %f\n", curCurv); }
  // Should we test track charge
  if (m_Charge_Sign*track[rowNumber].icharge >= 0) {
    // Selects only the tracks with the "large" curvature (> 0.1)
    if (track[rowNumber].curvature > 0.01) {
      if (colorIndex > 6) colorIndex = 0;
      return kRed+colorIndex;
    }
  }
  return 0;
}
