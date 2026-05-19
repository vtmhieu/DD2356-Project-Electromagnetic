
real	0m11.264s
user	0m11.251s
sys	0m0.008s

 Performance counter stats for './originalC':

        13,175,407      cache-misses:u                   #    0.801 % of all cache refs    
     1,645,551,667      cache-references:u                                                 
    51,222,765,885      L1-dcache-loads:u                                                  
       801,149,907      L1-dcache-load-misses:u          #    1.56% of all L1-dcache accesses
    35,905,756,922      cycles:u                                                           
    70,412,692,558      instructions:u                   #    1.96  insn per cycle         

      11.234195884 seconds time elapsed

      11.233152000 seconds user
       0.000000000 seconds sys



 Performance counter stats for './originalC':

       801,024,522      L1-dcache-load-misses:u                                            
   <not supported>      L1-dcache-store-misses:u                                    
   <not supported>      LLC-loads:u                                                 
   <not supported>      LLC-load-misses:u                                           

      11.243842795 seconds time elapsed

      11.239082000 seconds user
       0.003999000 seconds sys



 Performance counter stats for './originalC':

     6,402,988,318      branch-instructions:u                                              
            70,723      branch-misses:u                  #    0.00% of all branches        

      11.239745873 seconds time elapsed

      11.229770000 seconds user
       0.007998000 seconds sys



 Performance counter stats for './originalC':

        12,569,313      dTLB-loads:u                                                       
             4,801      dTLB-load-misses:u               #    0.04% of all dTLB cache accesses
   <not supported>      dTLB-stores:u                                               
   <not supported>      dTLB-store-misses:u                                         

      11.231479278 seconds time elapsed

      11.222748000 seconds user
       0.007999000 seconds sys


[ perf record: Woken up 16 times to write data ]
[ perf record: Captured and wrote 3.882 MB perf.data (46195 samples) ]
==3558280== Cachegrind, a cache and branch-prediction profiler
==3558280== Copyright (C) 2002-2017, and GNU GPL'd, by Nicholas Nethercote et al.
==3558280== Using Valgrind-3.20.0 and LibVEX; rerun with -h for copyright info
==3558280== Command: ./originalC
==3558280== 
--3558280-- warning: L3 cache found, using its data for the LL simulation.
==3558280== brk segment overflow in thread #1: can't grow to 0x485f000
==3558280== (see section Limitations in user manual)
==3558280== NOTE: further instances of this message will not be shown
==3558280== 
==3558280== I   refs:      70,412,879,083
==3558280== I1  misses:             2,779
==3558280== LLi misses:             2,479
==3558280== I1  miss rate:           0.00%
==3558280== LLi miss rate:           0.00%
==3558280== 
==3558280== D   refs:      51,205,035,985  (44,803,572,029 rd   + 6,401,463,956 wr)
==3558280== D1  misses:       800,169,470  (   800,144,060 rd   +        25,410 wr)
==3558280== LLd misses:            40,900  (        17,053 rd   +        23,847 wr)
==3558280== D1  miss rate:            1.6% (           1.8%     +           0.0%  )
==3558280== LLd miss rate:            0.0% (           0.0%     +           0.0%  )
==3558280== 
==3558280== LL refs:          800,172,249  (   800,146,839 rd   +        25,410 wr)
==3558280== LL misses:             43,379  (        19,532 rd   +        23,847 wr)
==3558280== LL miss rate:             0.0% (           0.0%     +           0.0%  )
/var/spool/slurm/slurmd/job20718245/slurm_script: line 36: syntax error near unexpected token `newline'
/var/spool/slurm/slurmd/job20718245/slurm_script: line 36: `cg_annotate cachegrind.out.<pid>'
