# Performance Tuning

## Trading Memory Usage for Speed

### Preload vs lazy-load meta data for the primary element in index permutations

#### Dev notes

Random Thoughts:

1) anything in RAM "per primary" (and thus "per object" for OXX) is too much!
2) have a new style <varSize-oldMeta><varSize-relIdToOffsetPairs><64b-startR2OPairs><64b-startMeta>
3) always build the full list, on startup choose to read the old Meta or not.
4) Without preload, do a binary search inside the file for RMD and cache RMD 
(just slowly grow what is otherwise preloaded) 

Downsides:

any access to OXX permutations requires log_2(400M) ~= 29 disk seeks. 
With very fast HDDs and especially SSDs this isn't ideal but acceptable (hopefully less due to cache)

For queries that join with ?s ?p ?o thr primary element of the permutation may be
predefined. It is probably not acceptable to use queries of this kind with the small-memory index.

Alternatives:
 
1) Use lots of RAM as before
2) Preload PXX and also SXX this time, because ?s ?p ?o queries are more often connected via ?s
3) Preload PXX and SPO and maybe also OSP, because one permutation can often be enough. Make sure the QueryPlanner uses this direction.

Conclusion:

Go with the strategy above and make the choice of preload vs no-preload possible for every permutation.
It should be possible to choose at startup without re-building the index.



### Externalize long vocabulary literals



## Prefix Search