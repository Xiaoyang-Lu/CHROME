//Hawkeye Cache Replacement Tool v2.0
//UT AUSTIN RESEARCH LICENSE (SOURCE CODE)
//The University of Texas at Austin has developed certain software and documentation that it desires to
//make available without charge to anyone for academic, research, experimental or personal use.
//This license is designed to guarantee freedom to use the software for these purposes. If you wish to
//distribute or make other use of the software, you may purchase a license to do so from the University of
//Texas.
///////////////////////////////////////////////
//                                            //
//     Hawkeye [Jain and Lin, ISCA' 16]       //
//     Akanksha Jain, akanksha@cs.utexas.edu  //
//                                            //
///////////////////////////////////////////////


#ifndef OPTGENML_H
#define OPTGENML_H

using namespace std;

#include <iostream>

#include <math.h>
#include <set>
#include <vector>

class Result
{
    // get predictions
    public:
			vector<int> prediction;
			int length;
			int current;
            		double confidence;
            		double threshold;

			Result(){
				this->length = 1;
				this->current = 0;
				this->prediction.push_back(false);
                    		this->confidence = 0;
                   		this->threshold = 0;
			}
			Result(int current, int length){
				this->length = length;
				this->current = current;
				prediction.clear();
				for (int i = 0; i < length; i++){
					prediction.push_back(0);
				}
                    		confidence = 0;
                    		threshold = 0;
			}

			~Result(){
				prediction.clear();
                    		confidence = 0;
                    		threshold = 0;
			}
				
			Result copy(){
				Result r(current, length);
				for (int i = 0; i < length; i++){
					r.prediction[i] = prediction[i];
				}
                   		r.confidence = confidence;
                    		r.threshold = threshold;
				return r;
			}
};


struct ADDR_INFO_ML
{
    uint64_t addr;
    uint32_t last_quanta;
    uint64_t PC; 
    uint64_t cpu;
    uint64_t set;
    bool prefetched;
    uint32_t lru;
    int last_prediction;
    bool is_high_cost_predicted; // for Obol
    uint64_t last_miss_cost;
    uint32_t index;
    vector<uint64_t> context;
    bool detrained;
    bool evicted;
    bool reuse;
    Result prev_result;

    void init(unsigned int curr_quanta, bool is_next_high_cost = false)
    {
        last_quanta = 0;
        PC = 0;
        prefetched = false;
        lru = 0;
        is_high_cost_predicted = is_next_high_cost;
        last_miss_cost = 0;
        index = 0;
        
	    detrained = false;
	    evicted = false;
        reuse = false;
        context.clear();
        cpu = 0;
        set = 0;
    }

    void update(unsigned int curr_quanta, uint64_t _pc, int prediction, bool is_next_high_cost = false)
    {
        last_quanta = curr_quanta;
        PC = _pc;
        last_prediction = prediction;
        is_high_cost_predicted = is_next_high_cost;
        detrained = false;
	evicted = false;
    }

    void mark_prefetch()
    {
        prefetched = true;
    }
};

struct OPTgen_ML
{
    vector<unsigned int> liveness_history;
    vector<unsigned int> liveness_history_stable;

    uint64_t num_cache;
    uint64_t num_dont_cache;
    uint64_t access;
    uint64_t prefetch;
    uint64_t prefetch_cachehit;

    uint64_t CACHE_SIZE;

    void init(uint64_t size)
    {
        num_cache = 0;
        num_dont_cache = 0;
        access = 0;
        CACHE_SIZE = size;
        prefetch = 0;
        prefetch_cachehit = 0;
    }

    void add_access(uint64_t curr_quanta, int cpu = 0)
    {
        access++;
        liveness_history.resize(curr_quanta+1);
        liveness_history_stable.resize(curr_quanta+1);
        // TODO: initialize liveness with 0 or 1?
        liveness_history[curr_quanta] = 0;
        liveness_history_stable[curr_quanta] = 0;
    }

    void add_prefetch(uint64_t curr_quanta)
    {
        liveness_history.resize(curr_quanta+1);
        liveness_history_stable.resize(curr_quanta+1);
        prefetch++;
        liveness_history[curr_quanta] = 0;
        liveness_history_stable[curr_quanta] = 0;
    }

    bool should_cache(uint64_t curr_quanta, uint64_t last_quanta, bool prefetch=false, int cpu = 0)
    {
        bool is_cache = true;
	//cout << "optgen_ml 1 before " << endl;
        assert(curr_quanta <= liveness_history.size());
	//cout << "optgen_ml 1 before " << endl;
        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        //if ((is_cache) && (last_quanta != curr_quanta))
        if ((is_cache))
        {
            i = last_quanta;
            while (i != curr_quanta)
            {
                liveness_history[i]++;
                liveness_history_stable[i]++;
                i++;
            }
     	       //cout << "optgen_ml 2 before " << endl;
	       assert(i == curr_quanta);
	       //cout << "optgen_ml 2 before " << endl;
        }

        if(!prefetch)
        {
            if (is_cache) num_cache++;
            else num_dont_cache++;
        }
        else
        {
            if(is_cache)
                prefetch_cachehit++;
        }

        return is_cache;    
    }

    bool should_cache_probe(uint64_t curr_quanta, uint64_t last_quanta)
    {
        bool is_cache = true;

        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        return is_cache;    
    }

    bool should_cache_tentative(uint64_t curr_quanta, uint64_t last_quanta)
    {
        bool is_cache = true;

        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        //if ((is_cache) && (last_quanta != curr_quanta))
        if ((is_cache))
        {
            i = last_quanta;
            while (i != curr_quanta)
            {
                liveness_history[i]++;
                i++;
            }
     		//cout << "optgen_ml 3 before " << endl;
	       assert(i == curr_quanta);
		//cout << "optgen_ml 3 after " << endl;
        }

        return is_cache;    
    }

    void revert_to_checkpoint()
    {
     	//cout << "optgen_ml 4 before " << endl;
	assert(liveness_history.size() == liveness_history_stable.size());
        //cout << "optgen_ml 4 after " << endl;
	for(unsigned int i=0; i < liveness_history.size(); i++)
            liveness_history[i] = liveness_history_stable[i];
    }

    uint64_t get_num_opt_accesses()
    {
        //assert((num_cache+num_dont_cache) == access);
        return access;
    }

    uint64_t get_num_opt_hits()
    {
        return num_cache;

        uint64_t num_opt_misses = access - num_cache;
        return num_opt_misses;
    }

    uint64_t get_traffic()
    {
        return (prefetch - prefetch_cachehit + access - num_cache);
    }

    void print()
    {
        cout << "Liveness Info " << liveness_history.size() << endl;        
        for(unsigned int i=0 ; i < liveness_history.size(); i++)
            cout << liveness_history[i] << ", ";

        cout << endl << endl;

        cout << "Liveness Stable Info " << liveness_history_stable.size() << endl;        
        for(unsigned int i=0 ; i < liveness_history_stable.size(); i++)
            cout << liveness_history_stable[i] << ", ";

        cout << endl << endl;
    }
};

#endif
