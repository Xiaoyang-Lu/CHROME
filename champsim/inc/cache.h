#ifndef CACHE_H
#define CACHE_H
#include <set>
#include <map>
#include "memory_class.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>


// PAGE
extern uint32_t PAGE_TABLE_LATENCY, SWAP_LATENCY;
#define SET_MAX 1000000

// CACHE TYPE
#define IS_ITLB 0
#define IS_DTLB 1
#define IS_STLB 2
#define IS_L1I  3
#define IS_L1D  4
#define IS_L2C  5
#define IS_LLC  6

// INSTRUCTION TLB
#define ITLB_SET 64
#define ITLB_WAY 4
#define ITLB_RQ_SIZE 16
#define ITLB_WQ_SIZE 16
#define ITLB_PQ_SIZE 0
#define ITLB_MSHR_SIZE 8
#define ITLB_LATENCY 1

// DATA TLB
#define DTLB_SET 16
#define DTLB_WAY 6
#define DTLB_RQ_SIZE 16
#define DTLB_WQ_SIZE 16
#define DTLB_PQ_SIZE 0
#define DTLB_MSHR_SIZE 8
#define DTLB_LATENCY 1

// SECOND LEVEL TLB
#define STLB_SET 128
#define STLB_WAY 16
#define STLB_RQ_SIZE 32
#define STLB_WQ_SIZE 32
#define STLB_PQ_SIZE 0
#define STLB_MSHR_SIZE 16
#define STLB_LATENCY 8

// L1 INSTRUCTION CACHE
#define L1I_SET 64
#define L1I_WAY 8
#define L1I_RQ_SIZE 64
#define L1I_WQ_SIZE 64 
#define L1I_PQ_SIZE 32
#define L1I_MSHR_SIZE 8
#define L1I_LATENCY 4

// L1 DATA CACHE
#define L1D_SET 64
#define L1D_WAY 12
#define L1D_RQ_SIZE 64
#define L1D_WQ_SIZE 64 
#define L1D_PQ_SIZE 8
#define L1D_MSHR_SIZE 16
#define L1D_LATENCY 5

// L2 CACHE
#define L2C_SET 1024
#define L2C_WAY 20
#define L2C_RQ_SIZE 48
#define L2C_WQ_SIZE 48
#define L2C_PQ_SIZE 16
#define L2C_MSHR_SIZE 48
#define L2C_LATENCY 10  // 4/5 (L1I or L1D) + 10 = 14/15 cycles

// LAST LEVEL CACHE
#define LLC_SET NUM_CPUS*4096
#define LLC_WAY 12
#define LLC_RQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_WQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_PQ_SIZE NUM_CPUS*32
#define LLC_MSHR_SIZE NUM_CPUS*64
#define LLC_LATENCY 40  // 4/5 (L1I or L1D) + 10 + 40 = 54/55 cycles

extern uint64_t LLC_MSHR;

void print_cache_config();

class Info
{
public:
    uint64_t accSeq;
    uint64_t pc;
    uint64_t offset;
    uint64_t page;
    uint64_t pc_offset;
    uint64_t pc_page;
    uint64_t pc_path;
    uint64_t pc_path_offset;
    uint64_t pc_path_page;
    uint64_t bypass;
    uint64_t bypass_algorithm;
};


class Stats {
  public:
    Stats() {
        for (int i = 0; i < NUM_CPUS; i += 1) {
            for (auto& metric : this->metrics) {
                this->roi_stats[i][metric] = 0;
                this->total_stats[i][metric] = 0;
            }
            this->cur_stats[i] = nullptr;
        }
    }

    void llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, uint32_t bypass_algorithm, uint32_t bypass, uint32_t random) {
        /* don't do anything if CPU is in warmup phase */
        if (!this->cur_stats[cpu])
            return;
        auto& stats = *this->cur_stats[cpu];

	if(hit)
	{
                for (int i = 0; i < NUM_CPUS; i += 1)
                {
                        std::map<uint64_t, uint32_t>::iterator it = this->important_blocks[i].find(full_addr);
                        if(it != this->important_blocks[i].end())
                        {
                                (*this->cur_stats[i])["Accurate"] += 1;
                                (*this->cur_stats[i])["Undecided"] -= 1;
                                if(it->second == 1)
                                {
                                        (*this->cur_stats[i])["Random_Accurate"] += 1;
    					(*this->cur_stats[i])["Random_Undecided"] -= 1;
                            	}
                                else
                                {
                                        (*this->cur_stats[i])["Non-Random_Accurate"] += 1;
                                	(*this->cur_stats[i])["Non-Random_Undecided"] -= 1;
				}
                                this->important_blocks[i].erase(it);
			}
		}
	}

	for (int i = 0; i < NUM_CPUS; i += 1)
	{
		std::map<uint64_t, uint32_t>::iterator it = this->important_blocks[i].find(victim_addr);
		if(it != this->important_blocks[i].end())
		{
			(*this->cur_stats[i])["Inaccurate"] += 1;
			(*this->cur_stats[i])["Undecided"] -= 1;
			if(it->second == 1)
                        {
                        	(*this->cur_stats[i])["Random_Inaccurate"] += 1;
                        	(*this->cur_stats[i])["Random_Undecided"] -= 1;
			}
                        else
                        {
                        	(*this->cur_stats[i])["Non-Random_Inaccurate"] += 1;
         			(*this->cur_stats[i])["Non-Random_Undecided"] -= 1;
	                }
                        this->important_blocks[i].erase(it);
		}
	}


	if(!hit)
	{
		for (int i = 0; i < NUM_CPUS; i += 1)
		{
			std::vector<pair<uint64_t, uint32_t>>::iterator it = this->vec_unimportant_blocks[i].begin();
                	for(uint32_t index = 0; index < this->vec_unimportant_blocks[i].size(); ++index)
			{
				if(this->vec_unimportant_blocks[i][index].first == full_addr)
				{
					(*this->cur_stats[i])["Inaccurate"] += 1;
					(*this->cur_stats[i])["Undecided"] -= 1;
					if(this->vec_unimportant_blocks[i][index].second == 1)
					{
						(*this->cur_stats[i])["Random_Inaccurate"] += 1;
						(*this->cur_stats[i])["Random_Undecided"] -= 1;
					}
					else
					{
						(*this->cur_stats[i])["Non-Random_Inaccurate"] += 1;
						(*this->cur_stats[i])["Non-Random_Undecided"] -= 1;
					}
					this->vec_unimportant_blocks[i].erase(it);

				}
				it++;
			}

        	}
	}

	for (int i = 0; i < NUM_CPUS; i += 1) {
		if(this->vec_unimportant_blocks[i].size() > 32768)
		{
			(*this->cur_stats[i])["Accurate"] += 1;
			(*this->cur_stats[i])["Undecided"] -= 1;
                        if(this->vec_unimportant_blocks[i].front().second == 1)
                        {
				(*this->cur_stats[i])["Random_Accurate"] += 1;
                        	(*this->cur_stats[i])["Random_Undecided"] -= 1;
			}
			else
			{
				(*this->cur_stats[i])["Non-Random_Accurate"] += 1;
				(*this->cur_stats[i])["Non-Random_Undecided"] -= 1;
			}
			this->vec_unimportant_blocks[i].erase(this->vec_unimportant_blocks[i].begin());
		}
	}


        if (bypass_algorithm == 1) {
            stats["Decisions"] += 1;
     	    stats["Undecided"] += 1;
	    if(bypass == 1)
	    {
		stats["Bypassing"] += 1;
		vec_unimportant_blocks[cpu].push_back(pair<uint64_t, uint32_t> (full_addr, random));
	    }
	    else
	    {
		stats["Replacement"] += 1;
		important_blocks[cpu].insert(pair<uint64_t, uint32_t> (full_addr, random));
	    }

	    if(random == 1)
	    {
		stats["Random"] += 1;
	    	stats["Random_Undecided"] += 1;
	    }
	    else
	    {
		stats["Non-Random"] += 1;
	    	stats["Non-Random_Undecided"] += 1;
	    }
        }
    }

    void llc_decision_inform_warmup_complete() {
        for (int i = 0; i < NUM_CPUS; i += 1)
            this->cur_stats[i] = &this->roi_stats[i];
    }

    void llc_decision_inform_roi_complete(uint32_t cpu) {
        this->total_stats[cpu] = this->roi_stats[cpu]; /* copy roi_stats over to total_stats */
        this->cur_stats[cpu] = &this->total_stats[cpu];
    }

    void llc_decision_final_stats(uint32_t cpu) {
        this->print_stats(this->total_stats, "Total", cpu);
    }

    void llc_decision_roi_stats(uint32_t cpu) {
        this->print_stats(this->roi_stats, "ROI", cpu);
    }

    void llc_decision_cur_stats(uint32_t cpu) {
        this->print_stats(*this->cur_stats, "Current", cpu);
    }

    void print_stats(unordered_map<string, uint64_t> stats[], string name, uint32_t cpu) {
        cout << "=== CPU " << cpu << " " << name << " Stats ===" << endl;
        for (auto &metric : this->metrics)
            cout << "* CPU " << cpu << " " << name << " " << metric << ": " << stats[cpu][metric] << endl;
    }




  private:
    vector<string> metrics = {"Decisions", "Bypassing", "Replacement", "Accurate", "Inaccurate", "Undecided", "Random", "Random_Accurate", "Random_Inaccurate", "Random_Undecided", "Non-Random", "Non-Random_Accurate", "Non-Random_Inaccurate", "Non-Random_Undecided"};
    unordered_map<string, uint64_t> roi_stats[NUM_CPUS], total_stats[NUM_CPUS];
    unordered_map<string, uint64_t> *cur_stats[NUM_CPUS];
    //std::map<uint64_t, uint32_t> unimportant_blocks[NUM_CPUS];
    std::map<uint64_t, uint32_t> important_blocks[NUM_CPUS];
    vector<pair<uint64_t, uint32_t>> vec_unimportant_blocks[NUM_CPUS];
    //unordered_set<uint64_t> inserted_blocks[NUM_CPUS];
    //map<uint64_t, uint64_t> bypassed_blocks[NUM_CPUS];
    //unordered_set<uint64_t> bypassed_blocks[NUM_CPUS];
};




class CACHE : public MEMORY {
  public:
    uint32_t cpu;
    const string NAME;
    const uint32_t NUM_SET, NUM_WAY, NUM_LINE, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
    uint32_t LATENCY;
    BLOCK **block;
    int fill_level;
    uint32_t MAX_READ, MAX_FILL;
    uint32_t reads_available_this_cycle;
    uint8_t cache_type; 
    Stats stats;    
    //pmc distribute
    uint64_t pmc_0_49,
                    pmc_50_99,
                    pmc_100_149,
                    pmc_150_199,
                    pmc_200_249,
                    pmc_250_299,
                    pmc_300_349,
                    pmc_350_above;
/*
                    pmc_700_799,
                    pmc_800_899,
                    pmc_900_999,
                    pmc_1000_1099,
                    pmc_1100_1199,
                    pmc_1200_1299,
                    pmc_1300_above;
*/   
    uint64_t MSHR_evit;
    uint64_t miss_total;
    // to measure cal
    uint64_t miss_count, last_miss_count, epoch_miss_count;
    float cal;
    uint32_t cal_level;
    float max_cal;
    uint64_t cal_level_hist[LLC_CAL_LEVELS];
    // prefetch stats
    uint64_t pf_requested,
             pf_issued,
             pf_useful,
             pf_useless,
	     pf_late,
             pf_fill;

    uint64_t re_count,
             by_count;
    uint64_t total;
    uint64_t accurate,
             inaccurate;
    deque<uint64_t> last_pcs;
    std::set<uint64_t> pcs;
    std::set<uint64_t> addrs;
    std::set<uint64_t> pages;
    std::set<uint64_t> offsets;
    std::set<uint64_t> pc_offset;
    std::set<uint64_t> pc_page;
    std::set<uint64_t> pc_path;
    std::set<uint64_t> pc_path_offset;
    std::set<uint64_t> pc_path_page;

   std::map<uint64_t, Info*> reuseHelper;

    int victim_lru[LLC_SET];
    // queues
    PACKET_QUEUE WQ{NAME + "_WQ", WQ_SIZE}, // write queue
                 RQ{NAME + "_RQ", RQ_SIZE}, // read queue
                 PQ{NAME + "_PQ", PQ_SIZE}, // prefetch queue
                 MSHR{NAME + "_MSHR", MSHR_SIZE}, // MSHR
                 PROCESSED{NAME + "_PROCESSED", ROB_SIZE}; // processed queue
    
    // camat: active cycles + active hit cycles + active miss cycles
    std::set<uint64_t> active_cycles;
    std::set<uint64_t> active_hit_cycles;
    std::set<uint64_t> active_miss_cycles;    
    std::map<uint32_t, set<uint64_t>> active_hit_cycles_per_core;
    std::map<uint32_t, set<uint64_t>> active_miss_cycles_per_core;
    std::map<uint32_t, set<uint64_t>> active_cycles_per_core;
    // pure miss reduced by prefetcher
    std::set<uint64_t> active_hit_cycles_wo_prefetch;

    std::map<uint64_t, uint32_t> MSHR_occupy[NUM_CPUS];
    
    uint64_t total_active_cycles,
             total_active_hit_cycles,
             total_active_miss_cycles,
             total_active_hit_cycles_wo_prefetch;
    
    uint64_t total_active_hit_cycles_per_core[NUM_CPUS],
	     total_active_miss_cycles_per_core[NUM_CPUS],
             total_active_cycles_per_core[NUM_CPUS];             
		
    uint64_t sim_access[NUM_CPUS][NUM_TYPES],
             sim_hit[NUM_CPUS][NUM_TYPES],
             sim_miss[NUM_CPUS][NUM_TYPES],
	     sim_overlap_miss[NUM_CPUS][NUM_TYPES],
             sim_pure_miss[NUM_CPUS][NUM_TYPES],
             sim_pure_miss_reduced_by_prefetch[NUM_CPUS][NUM_TYPES],
             roi_access[NUM_CPUS][NUM_TYPES],
             roi_hit[NUM_CPUS][NUM_TYPES],
             roi_miss[NUM_CPUS][NUM_TYPES],
             roi_overlap_miss[NUM_CPUS][NUM_TYPES],
	     roi_pure_miss[NUM_CPUS][NUM_TYPES],
             roi_pure_miss_reduced_by_prefetch[NUM_CPUS][NUM_TYPES];

    uint64_t total_miss_latency;
    
    // constructor
    CACHE(string v1, uint32_t v2, int v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8) 
        : NAME(v1), NUM_SET(v2), NUM_WAY(v3), NUM_LINE(v4), WQ_SIZE(v5), RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8) {

        LATENCY = 0;

        // cache block
        block = new BLOCK* [NUM_SET];
        for (uint32_t i=0; i<NUM_SET; i++) {
            block[i] = new BLOCK[NUM_WAY]; 

            for (uint32_t j=0; j<NUM_WAY; j++) {
                block[i][j].lru = j;
            }
        }

	for (uint32_t i=0; i<NUM_CPUS; i++)
	{
		if(active_hit_cycles_per_core.find(i) == active_hit_cycles_per_core.end())
		{
			active_hit_cycles_per_core[i] = set<uint64_t>();
		}
		if(active_miss_cycles_per_core.find(i) == active_miss_cycles_per_core.end())
		{
			active_miss_cycles_per_core[i] = set<uint64_t>();
		}
		if(active_cycles_per_core.find(i) == active_cycles_per_core.end())
		{
			active_cycles_per_core[i] = set<uint64_t>();
		}
	}

        for (uint32_t i=0; i<NUM_CPUS; i++) {
            upper_level_icache[i] = NULL;
            upper_level_dcache[i] = NULL;

            for (uint32_t j=0; j<NUM_TYPES; j++) {
                sim_access[i][j] = 0;
                sim_hit[i][j] = 0;
                sim_miss[i][j] = 0;
		sim_overlap_miss[i][j] = 0;
                sim_pure_miss[i][j] = 0;                
                sim_pure_miss_reduced_by_prefetch[i][j] = 0;
                roi_access[i][j] = 0;
                roi_hit[i][j] = 0;
                roi_miss[i][j] = 0;
                roi_pure_miss[i][j] = 0;
                roi_pure_miss_reduced_by_prefetch[i][j] = 0;
            }
        }

	    total_miss_latency = 0;

        lower_level = NULL;
        extra_interface = NULL;
        fill_level = -1;
        MAX_READ = 1;
        MAX_FILL = 1;

        pf_requested = 0;
        pf_issued = 0;
        pf_useful = 0;
        pf_useless = 0;
        pf_late = 0;
        pf_fill = 0;

	//pmc distribute
	pmc_0_49 = 0;
	pmc_50_99 = 0;
	pmc_100_149 = 0;
	pmc_150_199 = 0;
	pmc_200_249 = 0;
	pmc_250_299 = 0;
	pmc_300_349 = 0;
        pmc_350_above = 0;
/*
	pmc_700_799 = 0;
	pmc_800_899 = 0;
	pmc_900_999 = 0;
	pmc_1000_1099 = 0;
	pmc_1100_1199 = 0;
	pmc_1200_1299 = 0;
	pmc_1300_above = 0;
*/
	MSHR_evit = 0;
	miss_total = 0;
	miss_count = 0;
	last_miss_count = 0;  
	epoch_miss_count = 0;
    	cal = 0;
	max_cal = 0;
        cal_level = 0;
	//total_active_cycles = 0;
        //total_active_hit_cycles = 0;
        //total_active_miss_cycles = 0;
        //total_active_hit_cycles_wo_prefetch = 0;
    };

    // destructor
    ~CACHE() {
        for (uint32_t i=0; i<NUM_SET; i++)
            delete[] block[i];
        delete[] block;
    };

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    int  check_hit(PACKET *packet),
         invalidate_entry(uint64_t inval_addr),
         check_mshr(PACKET *packet),
         prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, uint32_t prefetch_metadata),
         kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata);

    void handle_fill(),
         handle_writeback(),
         handle_read(),
         handle_prefetch();

    void add_mshr(PACKET *packet),
         update_fill_cycle(),
         llc_initialize_replacement(),
         update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, uint32_t action),
         lru_update(uint32_t set, uint32_t way),
         fill_cache(uint32_t set, uint32_t way, PACKET *packet),
         replacement_final_stats(),
         llc_replacement_final_stats(),
         l1d_prefetcher_initialize(),
         l2c_prefetcher_initialize(),
         llc_prefetcher_initialize(),
         prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr),
         l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         l1d_prefetcher_final_stats(),
         l2c_prefetcher_final_stats(),
         llc_prefetcher_final_stats();

    uint32_t l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         llc_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         llc_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);

    void prefetcher_feedback(uint64_t &pref_gen, uint64_t &pref_fill, uint64_t &pref_used, uint64_t &pref_late);

    uint32_t get_set(uint64_t address),
             get_way(uint64_t address, uint32_t set),
             find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             llc_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type, uint32_t action),
             lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);

    std::vector<uint32_t> llc_bypassing_decision (uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, uint64_t pure_miss);
    void llc_initialize_bypassing(bool zero_init);

    void llc_decision_final_stats(uint32_t cpu),
         llc_decision_inform_warmup_complete(),
         llc_decision_inform_roi_complete(uint32_t cpu),
         llc_decision_roi_stats(uint32_t cpu);

    void broadcast_bw(uint8_t bw_level),
        llc_replacement_broadcast_bw(uint8_t bw_level);


    void broadcast_cal_level(uint32_t cal_level),
        llc_replacement_broadcast_cal_level(uint32_t cal_level);

    void  obstruction_candidate(uint32_t cpu);
    uint32_t llc_management_decision(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, uint64_t obstruction[NUM_CPUS]);
};

#endif
