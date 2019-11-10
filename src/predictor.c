//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Sumiran Shubhi";
const char *studentID   = "A53314039";
const char *email       = "sshubhi@eng.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//
uint8_t	*gshare_bht;
uint8_t *localpred_bht;
uint8_t	*globalpred_bht;
uint8_t *choice_predictor;  
uint32_t *localhist_table;
uint32_t ghr;

uint8_t bht_mask = 3;
uint32_t localhist_mask;
uint32_t pc_index_mask;
uint32_t globalhist_mask;




//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

uint8_t get_prediction_2b_cntr(uint8_t *cntr, uint32_t index)
{
	uint8_t mask = 2;
	return ((*(cntr+index) & mask)>>1);
}

void update_2b_cntr(uint8_t *cntr, uint32_t index, uint8_t outcome)
{
	if ((outcome == 1) & (*(cntr + index) != 3))
		*(cntr+index) = (*(cntr + index) + 1) & bht_mask;
	else if ((outcome == 0) & (*(cntr + index) != 0))
		*(cntr+index) = (*(cntr+index)+1) & bht_mask;
}

uint32_t get_local_pred_pattern(uint32_t *cntr, uint32_t index)
{
	return (*(cntr + index) & localhist_mask);
}

void update_local_pred_pattern(uint32_t *cntr, uint32_t index, uint8_t outcome)
{
	*(cntr + index) = ((*(cntr+index) << 1) | outcome) & localhist_mask;
}

// Initialize the predictor
//
void
init_predictor()
{
  localhist_mask = (1<<lhistoryBits) - 1;
  pc_index_mask = (1<<pcIndexBits) - 1;
  globalhist_mask = (1<<ghistoryBits) - 1;
  ghr = 0;
  gshare_bht = (uint8_t *)calloc(1, (pc_index_mask + 1)*sizeof(uint8_t));
  localpred_bht = (uint8_t *)calloc(1, (localhist_mask+1)*sizeof(localpred_bht));
  globalpred_bht = (uint8_t *)calloc(1, (globalhist_mask+1)*sizeof(globalpred_bht));
  choice_predictor = (uint8_t *)calloc(0, (globalhist_mask+1)*sizeof(choice_predictor));
  localhist_table = (uint32_t *)calloc(2, (pc_index_mask+1)*sizeof(localhist_table)); 
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{	uint8_t	prediction;

    //Gshare
	uint32_t pc_xor_ghr = (pc ^ ghr) & pc_index_mask;
	
	//Tournament
	uint32_t pc_index = pc & pc_index_mask;
	uint32_t localpred_index = get_local_pred_pattern(localhist_table,pc_index);
    uint32_t globalpred_index = ghr & globalhist_mask;
	
	//Custom

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE: {
		return (get_prediction_2b_cntr(gshare_bht,pc_xor_ghr));
	}
    case TOURNAMENT: {
		prediction = (get_prediction_2b_cntr(choice_predictor,globalpred_index)) ? get_prediction_2b_cntr(globalpred_bht,globalpred_index) : get_prediction_2b_cntr(localpred_bht,localpred_index);
	}
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  printf("Error! No compatible bpType");
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
	   //Gshare
	uint32_t pc_xor_ghr = (pc ^ ghr) & pc_index_mask;
	
	//Tournament
	uint32_t pc_index = pc & pc_index_mask;
	uint32_t localpred_index = get_local_pred_pattern(localhist_table,pc_index);
    uint32_t globalpred_index = ghr & globalhist_mask;
	
	update_2b_cntr(gshare_bht,pc_xor_ghr,outcome);
	update_2b_cntr(localpred_bht,localpred_index,outcome);
	update_2b_cntr(globalpred_bht,globalpred_index,outcome);
	
	//Update choice_predictor 
	//update_2b_cntr(choice_predictor,globalpred_bht,outcome);

	ghr = (ghr << 1) | outcome;
	update_local_pred_pattern(localhist_table,pc_index,outcome);
	
}
