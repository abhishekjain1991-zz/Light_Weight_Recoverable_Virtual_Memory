Group Members
1) Abhishek Jain
Special Request 
**************************************************************************************
During compilation, we need to enable the flag -std=c++11.
**************************************************************************************

1) How you use logfiles to accomplish persistency plus transaction semantics?
Ans: The logfile holds the data for one segment. Every transaction has the format:- 
	1) Offset
	2) Size
	3) Data
	Log files and segment files are saved with name same as segment names followed by .seg or .log respectively.
	The transaction is saved in the ".log" file when the transaction is committed. 
	On a call to trucation, any log file stored in the directory for the rim object is combined with the segment file of the corresponding name to give a newly updated segment file.
	The log files are then deleted.
	If a process crashes, it can re obtain its data from the set file (here tract is called  first to update all segment files in backing store)
     	If there is any previous transaction that has been committed and truncated, you can obtain that from the ".seg" file.
     	If there is any new transaction which has just been committed, then you can obtain that from the ".log" file.

2) What goes in them? How do the files get cleaned up, so that they do not expand indefinitely?
Ans: 
	The ".log" file is created when the transactions are committed. On truncation, these transactions are put into the ".seg" file and the ".log" file is deleted.
	Now, suppose there exists a ".log" and ".seg" file for a particular segment.
     	If we call a truncate for this transaction, the ".seg" file is appended with the details of the ".log" file and the ".log" file is deleted.
     	This helps in the log file not expanding indefinitely and hence saves space.
	A single call to truncate removes all log files and updates all corresponding segment files.