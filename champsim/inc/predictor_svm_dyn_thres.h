#ifndef PREDICTOR_H
#define PREDICTOR_H

using namespace std;

#include <iostream>

#include <math.h>
#include <set>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include "cache.h"
#include "optgen_ml.h"


double my_interval_size = 8000;
int is_hawkeye = 0;
int my_counter_width = 8;
double pos_neg_ratio = 0;

uint64_t CRC( uint64_t _blockAddress )
{
    static const unsigned long long crcPolynomial = 3988292384ULL;
    unsigned long long _returnVal = _blockAddress;
    for( unsigned int i = 0; i < 32; i++ )
        _returnVal = ( ( _returnVal & 1 ) == 1 ) ? ( ( _returnVal >> 1 ) ^ crcPolynomial ) : ( _returnVal >> 1 );
    return _returnVal;
}


class HAWKEYE_PC_PREDICTOR
{
    map<uint64_t, short unsigned int > SHCT;

       public:

    void increment (uint64_t pc)
    {
        uint64_t signature = CRC(pc) % SHCT_SIZE;
        if(SHCT.find(signature) == SHCT.end())
            SHCT[signature] = (1+MAX_SHCT)/2;

        SHCT[signature] = (SHCT[signature] < MAX_SHCT) ? (SHCT[signature]+1) : MAX_SHCT;

    }

    void saturate (uint64_t pc)
    {
        uint64_t signature = CRC(pc) % SHCT_SIZE;
    	//cout << "predictor_svm_dyn_thres_1 before " << endl;
        assert(SHCT.find(signature) != SHCT.end());
	//cout << "predictor_svm_dyn_thres_1 after " << endl;
        SHCT[signature] = MAX_SHCT;

    }


    void decrement (uint64_t pc)
    {
        uint64_t signature = CRC(pc) % SHCT_SIZE;
        if(SHCT.find(signature) == SHCT.end())
            SHCT[signature] = (1+MAX_SHCT)/2;
        if(SHCT[signature] != 0)
            SHCT[signature] = SHCT[signature]-1;
    }

    bool get_prediction (uint64_t pc)
    {
        uint64_t signature = CRC(pc) % SHCT_SIZE;
        if(SHCT.find(signature) != SHCT.end() && SHCT[signature] < ((MAX_SHCT+1)/2))
            return false;
        return true;
    }

    short unsigned int get_probability (uint64_t pc)
    {
        uint64_t signature = CRC(pc) % SHCT_SIZE;
        if(SHCT.find(signature) != SHCT.end())
            return SHCT[signature];
        return -1;
    }
};

class BASE_PERCEPTRON
{
public:
    map<uint64_t, map<uint64_t, double>> weights;
    map<uint64_t, double> bias;

    double training_threshold; 
    int positive_count = 0; 
    int max_weight = (1 << (my_counter_width)) - 1;
    int min_weight = -(1 << (my_counter_width));

    void print_perceptron(uint64_t X, vector<uint64_t> pc_history)
    {
        set<uint64_t> s(pc_history.begin(), pc_history.end());
        cout << " " << bias[X] << "   ";
        for(auto iter = s.begin(); iter != s.end(); iter++)
        {
            uint64_t A = *iter;
            cout << weights[X][A] << " ";
        }
        cout << endl;
    }

    double dot_product(uint64_t X, set<uint64_t>& s)
    {
        double d_product = bias[X];
        for(auto iter = s.begin(); iter != s.end(); iter++)
        {
            uint64_t A = *iter;
            if(weights[X].find(A) == weights[X].end())
            {
                weights[X][A] = 0;
            }
            d_product += weights[X][A];
        }
        return d_product;
    }

    void update(uint64_t X, int32_t coeff, set<uint64_t>& s)
    {
        bias[X] += coeff;
        for(auto iter = s.begin(); iter != s.end(); iter++)
        {
            uint64_t A = *iter;
            weights[X][A] = weights[X][A] + coeff;
            if(weights[X][A] > max_weight)
                weights[X][A] = max_weight;
            if(weights[X][A] < min_weight)
                weights[X][A] = min_weight;
        }
    }

    void copy(BASE_PERCEPTRON* target){
        weights = target->weights;
        bias = target->bias;
        training_threshold = target->training_threshold;
        positive_count = target->positive_count;
    }
};

class MULTIPLE_PERCEPTRON_PREDICTOR
{
public:

    MULTIPLE_PERCEPTRON_PREDICTOR(int num_agent){
        this->num_agent = num_agent;
        agent = new BASE_PERCEPTRON[num_agent];
        agent[0].training_threshold = 0.0;
        for(int i = 0; i <= 10; i++)
            agent[i].training_threshold = i;
        for(int i = 11; i < 20; i++)
            agent[i].training_threshold = agent[i-1].training_threshold + 10;
        for(int i = 20; i < 30; i++)
            agent[i].training_threshold = agent[i-1].training_threshold + 100;
        for(int i = 31; i < 40; i++)
            agent[i].training_threshold = agent[i-1].training_threshold + 1000;

        current = num_agent / 2;
        last_conf = 0;

        cout << "Dynamic Perceptron" << endl;
        cout << "Interval size: " << my_interval_size << endl;
        cout << "Is Hawkeye baseline? " << is_hawkeye << endl;
        cout << "Counter width: " << my_counter_width << endl;
        cout << "only threshold: " << agent[0].training_threshold << endl;
    }

    HAWKEYE_PC_PREDICTOR* baseline_hawkeye_predictor = new HAWKEYE_PC_PREDICTOR();

    double step = 10;
    map<uint64_t, bool> paddr_this_interval; 
    int training_count = 0;  
    double past_thresholds[100000]; 
    int num_intervals = 0;
    int num_agent;
    int current;
    bool debug = false;
    double last_conf;

    BASE_PERCEPTRON* agent;

    void update_threshold()
    {
        past_thresholds[num_intervals] = agent[current].training_threshold;
        num_intervals++;

        paddr_this_interval.clear();
        training_count = 0;
        
        int max_id = current;
        for (int i = 0; i < num_agent; i++){
            if (agent[i].positive_count > agent[max_id].positive_count){
                max_id = i;
            }
        }
 	//cout << "predictor_svm_dyn_thres_2 before " << endl;
        assert(max_id != -1);
        //cout << "predictor_svm_dyn_thres_2 after " << endl;
        current = max_id;
    	for (int i = 0; i < num_agent; i++){
    		agent[i].positive_count = 0;
    	}
    }

    void increment (
        uint64_t pc, 
        vector<uint64_t> pc_history, 
        uint64_t paddr, 
        Result& prev_result, 
		bool detrained)
    {
        baseline_hawkeye_predictor->increment(pc);

        if(is_hawkeye)
            return;

        if(pc_history.size() == 0)
            return;

        if (training_count == my_interval_size){
            update_threshold();
        }

        bool inside_interval = (paddr_this_interval.find(paddr) != paddr_this_interval.end());

    	if(inside_interval){
    		training_count++;
    		if (detrained){
    			training_count--;
    		}
    	}

        uint64_t X = pc;
        bool ground_truth = true;
        int32_t coeff = ground_truth ? 1 : -1;
        double d_product;
        bool prediction;

        if(agent[current].weights.find(X) == agent[current].weights.end())
        {
            for (int i = 0; i < num_agent; i++){
                agent[i].weights[X].clear();
                agent[i].bias[X] = 0;
            }
        }

        set<uint64_t> s(pc_history.begin(), pc_history.end());

    	for (int i = 0; i < num_agent; i++){
    		d_product = agent[i].dot_product(X, s);
    		prediction = d_product > 0;
    		if(prediction == ground_truth && abs(d_product) >= agent[i].training_threshold) 
    			;
    		else
    			agent[i].update(X, coeff, s);
    		if (inside_interval){
    			if (detrained){
    				if (prev_result.prediction[i] == false){
    					agent[i].positive_count--;
    				}
    			}
    			if (prev_result.prediction[i] == ground_truth){
    				agent[i].positive_count++;
    			}
    		}
    	}
    }

    void decrement (
        uint64_t pc, 
        vector<uint64_t> pc_history, 
        uint64_t paddr, 
        Result& prev_result,
	    bool detrained)
    {
        baseline_hawkeye_predictor->decrement(pc);

        if(is_hawkeye)
            return;

        if(pc_history.size() == 0) 
            return;

        if (training_count == my_interval_size)
            update_threshold();

        bool inside_interval = (paddr_this_interval.find(paddr) != paddr_this_interval.end());

        uint64_t X = pc;
        bool ground_truth = false;
        int32_t coeff = ground_truth ? 1 : -1;
        double d_product;
        bool prediction;
        if(inside_interval){
            training_count++;
	    	if (detrained){
				training_count--;
			}
		}

        if(agent[current].weights.find(X) == agent[current].weights.end())
        {
            for (int i = 0; i < num_agent; i++){
                agent[i].weights[X].clear();
                agent[i].bias[X] = 0;
            }
        }

        set<uint64_t> s(pc_history.begin(), pc_history.end());
        
        for (int i = 0; i < num_agent; i++){
            d_product = agent[i].dot_product(X, s);
            prediction = d_product > 0;
            if(prediction == ground_truth && abs(d_product) >= agent[i].training_threshold) 
                ;
            else
                agent[i].update(X, coeff, s);
			if (inside_interval){
				if (detrained){
					if (prev_result.prediction[i] == false){
						agent[i].positive_count--;
					}
				}
				if (prev_result.prediction[i] == ground_truth){
					agent[i].positive_count++;
				}
			}
        }
    }

    void fake_detraining_for_scan(
        uint64_t paddr, 
        Result& prev_result,
        bool detrained)
    {
        bool inside_interval = (paddr_this_interval.find(paddr) != paddr_this_interval.end());
        
        if(training_count == my_interval_size)
            update_threshold();
        
        if(inside_interval)
        {
            training_count++;
            if(detrained)
            {
                //cout << "predictor_svm_dyn_thres_3 before " << endl;
		assert(0);
		//cout << "predictor_svm_dyn_thres_3 after " << endl;
                training_count--;
            }
        }

        bool ground_truth = false;

        for(int i = 0; i < num_agent; i++)
        {
            if (inside_interval)
            {
                if(detrained)
                {
                    //cout << "predictor_svm_dyn_thres_4 before " << endl;
		    assert(0);
		    //cout << "predictor_svm_dyn_thres_4 after " << endl;
                    if (prev_result.prediction[i] == false){
                        agent[i].positive_count--;
                    }
                }
                if (prev_result.prediction[i] == ground_truth){
                    agent[i].positive_count++;
                }
            }
        }

        if (debug){
            cout << "fake detraining done" << endl;
        }
    }

    void print_perceptron (uint64_t pc, vector<uint64_t> pc_history)
    {
        agent[current].print_perceptron(pc, pc_history);
    }

    Result get_prediction (uint64_t pc, vector<uint64_t> pc_history, uint64_t paddr, double last_conf)
    {
        bool baseline_prediction = baseline_hawkeye_predictor->get_prediction(pc);
        if(is_hawkeye || pc_history.size() == 0 || agent[current].weights.find(pc) == agent[current].weights.end())
        {
            Result result(current, num_agent);
            for (int i = 0; i < num_agent; i++){
                result.prediction[i] = baseline_prediction;
            }
            return result;
        }
        
        if(is_hawkeye)
        {    
		//cout << "predictor_svm_dyn_thres_5 before " << endl;
		assert(0);
		//cout << "predictor_svm_dyn_thres_5 after " << endl;
	}
        if(paddr_this_interval.find(paddr) == paddr_this_interval.end())
            paddr_this_interval[paddr] = true;

        uint64_t X = pc;
        set<uint64_t> s(pc_history.begin(), pc_history.end());
        
        Result result(current, num_agent);
        for (int i = 0; i < num_agent; i++){
            int prediction = -1;
            double conf = agent[i].dot_product(X, s);
            if(conf > agent[i].training_threshold * pos_neg_ratio)
                prediction = 1;
            else if(conf > 0)
                prediction = 2;
            else
                prediction = 0;
	    
            result.prediction[i] = prediction;
        }
        return result;
    }
};

class MULTI_CORE_MULTIPLE_PERCEPTRON_PREDICTOR
{
public:
    MULTI_CORE_MULTIPLE_PERCEPTRON_PREDICTOR(int num_cpus, int num_agent){
        for(int i = 0; i < num_cpus; i++)
            single_core_predictors[i] = new MULTIPLE_PERCEPTRON_PREDICTOR(num_agent);
    }
    MULTIPLE_PERCEPTRON_PREDICTOR* single_core_predictors[16];
};

#endif
