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

//hgp -> hybrid gshare perceptron 
uint8_t *hgp_gshare_bht;
uint8_t *hgp_choice_predictor;
int8_t *hgp_perceptron_table;
uint32_t hgp_ghr_size = 14;
uint32_t hgp_perceptron_table_index_width = 7;
uint32_t hgp_perceptron_table_size;
uint32_t hgp_num_weight;
int32_t theta;


uint8_t bht_mask = 3;
uint32_t localhist_mask;
uint32_t pc_index_mask;
uint32_t globalhist_mask;
uint32_t hgp_ghr_mask;
uint32_t hgp_perceptron_table_mask;




//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

uint8_t get_prediction_2b_cntr(uint8_t *cntr, uint32_t index)
{	uint8_t mask = 2;	
	return ((*(cntr+index) & mask)>>1);
}

void update_2b_cntr(uint8_t *cntr, uint32_t index, uint8_t outcome)
{	
	if ((outcome == 1) & (*(cntr + index) != 3))
		*(cntr+index) = (*(cntr + index) + 1) & bht_mask;
	else if ((outcome == 0) & (*(cntr + index) != 0))
		*(cntr+index) = (*(cntr+index)-1) & bht_mask;
}

uint32_t get_local_pred_pattern(uint32_t *cntr, uint32_t index)
{
	return (*(cntr + index) & localhist_mask);
}

void update_local_pred_pattern(uint32_t *cntr, uint32_t index, uint8_t outcome)
{
	*(cntr + index) = ((*(cntr+index) << 1) | outcome) & localhist_mask;
}

uint8_t perceptron_predict (uint32_t pc)
{	uint8_t prediction;
	int perceptron_output;
	uint32_t pc_indexed = ( pc )& hgp_perceptron_table_mask;
	for (int i=0; i<hgp_num_weight; i++) {
		if (i == 0) {
			perceptron_output = *(hgp_perceptron_table + pc_indexed);
		}
		else {
			uint32_t temp;
			temp = (ghr >> (i-1)) & 1;
			if (temp) {
				perceptron_output += *(hgp_perceptron_table + pc_indexed + i);
			}
			else {
				perceptron_output -= *(hgp_perceptron_table + pc_indexed + i);
			}
		}
	}
	if (perceptron_output >= 0)
		prediction = 1;
	else 
		prediction = 0;
	return prediction;	
}

int perceptron_calc_output (uint32_t pc)
{
	int perceptron_output;
	uint32_t pc_indexed = (pc ) & hgp_perceptron_table_mask;
	
	for (int i=0; i<hgp_num_weight; i++) {
		if (i == 0) 
			perceptron_output = *(hgp_perceptron_table + pc_indexed);
		else {
			uint32_t temp;
			temp = (ghr >> (i-1)) & 1;
			if (temp)
				perceptron_output += *(hgp_perceptron_table + pc_indexed + i);
			else
				perceptron_output -= *(hgp_perceptron_table + pc_indexed + i);
		}
	}
	return perceptron_output;	
}
void perceptron_train (uint32_t pc, uint8_t outcome){
	uint32_t pc_indexed = (pc )& hgp_perceptron_table_mask;	
	if ((perceptron_predict(pc) != outcome) || (abs(perceptron_calc_output(pc)) < theta)) {
		for (int i=0; i<hgp_num_weight; i++) {
			if (i == 0) {
				if (outcome){
					*(hgp_perceptron_table + pc_indexed) = *(hgp_perceptron_table + pc_indexed) + 1;
				}
				else {
					*(hgp_perceptron_table + pc_indexed) = *(hgp_perceptron_table + pc_indexed) - 1;	
				}
			}	
			else {
				uint32_t temp;
				temp = (ghr >> (i-1)) & 1;
				if (temp == outcome){
					if (*(hgp_perceptron_table + pc_indexed + i) != 127){	

						*(hgp_perceptron_table + pc_indexed + i) = *(hgp_perceptron_table + pc_indexed + i) + 1;
					}
				}
				else {
					if (*(hgp_perceptron_table + pc_indexed + i) != -128) {
						*(hgp_perceptron_table + pc_indexed + i) = *(hgp_perceptron_table + pc_indexed + i) - 1;
					}
				}
			}
		}		
	}
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
  gshare_bht = (uint8_t *)calloc((globalhist_mask + 1), sizeof(uint8_t));
  localpred_bht = (uint8_t *)calloc((localhist_mask+1),sizeof(uint8_t));
  globalpred_bht = (uint8_t *)calloc((globalhist_mask+1),sizeof(uint8_t));
  choice_predictor = (uint8_t *)calloc((globalhist_mask+1),sizeof(uint8_t));
  localhist_table = (uint32_t *)calloc((pc_index_mask+1),sizeof(uint32_t)); 
  for (int i=0; i<globalhist_mask+1; i++){
	  *(gshare_bht+i) = 1;
	  *(globalpred_bht + i) = 1;
	  *(choice_predictor + i) = 2;
  }
  for (int i=0; i<localhist_mask+1; i++){
	  *(localpred_bht + i) = 1;
  }
  for (int i=0; i<pc_index_mask+1; i++){
	  *(localhist_table + i) = 0;	
  }
  //Custom Predictor
  hgp_num_weight = hgp_ghr_size + 1;
  theta = (1.93*hgp_ghr_size) + 14;
  hgp_ghr_mask = hgp_ghr_size - 1;
  hgp_perceptron_table_size = (1<<hgp_perceptron_table_index_width);
  hgp_perceptron_table_mask = hgp_perceptron_table_size - 1;
  
  hgp_gshare_bht = (uint8_t *)calloc((hgp_ghr_size), sizeof(uint8_t));
  hgp_choice_predictor = (uint8_t *)calloc((hgp_ghr_size), sizeof(uint8_t));
  hgp_perceptron_table = (int8_t *)calloc((hgp_perceptron_table_size)*hgp_num_weight, sizeof(int8_t));

  for (int i=0; i<hgp_ghr_size;i++) {
	  *(hgp_gshare_bht + i) = 1;
	  *(hgp_choice_predictor + i) = 2;
  }
  for (int i=0; i<hgp_perceptron_table_size;i++) {
	  for (int j=0; j<hgp_num_weight; j++){
	  *(hgp_perceptron_table + i + j) = -1;
	  }
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{	uint8_t	prediction;

    //Gshare
	uint32_t pc_xor_ghr = (pc & globalhist_mask) ^ (ghr & globalhist_mask);

	//Tournament
	uint32_t pc_index = pc & pc_index_mask;
	uint32_t localpred_index = get_local_pred_pattern(localhist_table,pc_index);
    uint32_t globalpred_index = ghr & globalhist_mask;
	
	//Custom
	uint32_t hgp_gshare_xor_index = (pc ^ ghr) & hgp_ghr_mask;
	uint32_t hgp_choice_pred_index = ghr & hgp_ghr_mask;

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE: {
		return (get_prediction_2b_cntr(gshare_bht,pc_xor_ghr));
	}
    case TOURNAMENT: {
		prediction = (get_prediction_2b_cntr(choice_predictor,globalpred_index)) ? get_prediction_2b_cntr(globalpred_bht,globalpred_index) : get_prediction_2b_cntr(localpred_bht,localpred_index);
		return prediction;
	}
    case CUSTOM: {
		prediction = (get_prediction_2b_cntr(hgp_choice_predictor,hgp_choice_pred_index)) ? perceptron_predict(pc) : get_prediction_2b_cntr(hgp_gshare_bht,hgp_gshare_xor_index);
		return prediction;
	}
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
 
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
	uint32_t pc_xor_ghr = (pc & globalhist_mask) ^ (ghr & globalhist_mask);
	
	//Tournament
	uint32_t pc_index = pc & pc_index_mask;
	uint32_t localpred_index = get_local_pred_pattern(localhist_table,pc_index);
    uint32_t globalpred_index = ghr & globalhist_mask;
	
	//Custom
	uint32_t hgp_gshare_xor_index = (pc ^ ghr) & hgp_ghr_mask;
	uint32_t hgp_choice_pred_index = ghr & hgp_ghr_mask;	
	
	//Update choice_predictor for tournament predictor
	if ((get_prediction_2b_cntr(localpred_bht,localpred_index) == outcome) && (get_prediction_2b_cntr(globalpred_bht,globalpred_index) != outcome)) {
		update_2b_cntr(choice_predictor,globalpred_index,(0));
	}
	else if ((get_prediction_2b_cntr(localpred_bht,localpred_index) != outcome) && (get_prediction_2b_cntr(globalpred_bht,globalpred_index) == outcome)) {
		update_2b_cntr(choice_predictor,globalpred_index,(1));
	}	
	
	//Update choice_predictor for custom predictor (hybrid gshare perceptron predictor)
	if ((get_prediction_2b_cntr(hgp_gshare_bht,hgp_gshare_xor_index) == outcome) && (perceptron_predict(pc) != outcome)) {
		update_2b_cntr(hgp_choice_predictor,hgp_choice_pred_index,(0));
	}
	else if ((get_prediction_2b_cntr(hgp_gshare_bht,hgp_gshare_xor_index) != outcome) && (perceptron_predict(pc) == outcome)) {
		update_2b_cntr(hgp_choice_predictor,hgp_choice_pred_index,(1));
	}	

	update_2b_cntr(gshare_bht,pc_xor_ghr,outcome);
	update_2b_cntr(localpred_bht,localpred_index,outcome);
	update_2b_cntr(globalpred_bht,globalpred_index,outcome);	
	update_2b_cntr(hgp_gshare_bht,hgp_gshare_xor_index,outcome);
	perceptron_train(pc,outcome);	
	ghr = (ghr << 1) | outcome;
	update_local_pred_pattern(localhist_table,pc_index,outcome);
	
}
