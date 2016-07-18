# Splitter classes:

```
Splitter_core 
    +- Splitter_IMS: Pure IP multicast
    +- Splitter_DBS: Pure IP unicast
	        +- Splitter_ACS: Chunk-rate adaptation to the peers capacity
			|      +- Splitter_ACS_LRS: Chunk-rate adaptation and lost-chunk recovery
			+- Splitter_LRS: Lost-chunk recovery
			+- Splitter_NTS: NAT traversal functionality
			       +- Splitter_ACS_NTS: 
			           
```
