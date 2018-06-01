/* resched.c  -  resched */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>
unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int scheduled_class = 0;

int resched()
{
	switch (getschedclass()) {
	case LINUXSCHED:
		return Linux_Scheduling();
	case RANDOMSCHED:
		return Random_Scheduling();
	default:
		return Default();
	}
	return OK;
}

void setschedclass(int sched_class) {
	scheduled_class = sched_class;
}

int getschedclass() {
	return scheduled_class;
}


int Linux_Scheduling(){
	register struct pentry  *optr;  /* pointer to old process entry */
        register struct pentry  *nptr;  /* pointer to new process entry */
	int epoch = 0;
	int max_goodness = 0;
        int newproc = 0;
	optr = &proctab[currpid];
	int Prio = optr->goodness - optr->counter;
        optr->goodness = Prio + preempt;
        optr->counter = preempt;
        if(optr->counter == 0 || currpid == NULLPROC){
               optr->goodness = 0;
               optr->counter = 0;
        }
        int i;
        for(i=q[rdytail].qprev; i != rdyhead; i=q[i].qprev){
             if(proctab[i].goodness > max_goodness){
                   newproc = i;
                   max_goodness = proctab[i].goodness;
             }
        }
	while(max_goodness == 0 && (optr->pstate != PRCURR || optr->counter == 0)){
		if(epoch == 0){
			epoch = 1;
			int j;
			struct pentry *p;
			for(j=0;j<NPROC;j++){
				p = &proctab[j];
				if (p->pstate == PRFREE)
					continue;
				else if(p->counter == p->quantum || p->counter == 0){
					p->quantum = p->pprio;
				}
				else{
					p->quantum = p->pprio + (p->counter)/2;
				}
				p->counter = p->quantum;
				p->goodness = p->counter + p->pprio;
			}
			preempt = optr->counter;		
			Prio = optr->goodness - optr->counter;
                	optr->goodness = Prio + preempt;
                	optr->counter = preempt;
                	if(optr->counter == 0 || currpid == NULLPROC){
                        	optr->goodness = 0;
                        	optr->counter = 0;
               		}
                	max_goodness = 0;
                	newproc = 0;
                	for(i=q[rdytail].qprev; i != rdyhead; i=q[i].qprev){
                        	if(proctab[i].goodness > max_goodness){
                                	newproc = i;
                               		max_goodness = proctab[i].goodness;
                       		}	
                	}	

	 	 }	
	}
	if(max_goodness == 0){
			if(currpid == NULLPROC){
				return OK;
			}
			else{
				newproc = NULLPROC;
				 if (optr->pstate == PRCURR) {
             		         	optr->pstate = PRREADY;
                		 	insert(currpid,rdyhead,optr->pprio);
       				 }
				nptr = &proctab[newproc];
				nptr->pstate = PRCURR;
				dequeue(newproc);
        			preempt = QUANTUM;
				currpid = newproc;
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return OK;
			}
	}
	else if(optr->goodness >= max_goodness && optr->goodness > 0 && optr->pstate == PRCURR){
		preempt = optr->counter;
		return OK;
	}
	else if(max_goodness > 0 && (optr->pstate != PRCURR || optr->counter == 0 || optr->goodness < max_goodness)){
		if (optr->pstate == PRCURR) {
                      optr->pstate = PRREADY;
                      insert(currpid,rdyhead,optr->pprio);
                 }
                 nptr = &proctab[newproc];
                 nptr->pstate = PRCURR;
                 dequeue(newproc);
                 preempt = QUANTUM;
                 currpid = newproc;
                 ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                 return OK;
	}
	else{
		return SYSERR;
	}
}

int Random_Scheduling(){
	int i;
	register struct pentry *optr;
	register struct pentry *nptr;
	int prio_sum = 0;
	int proc = 0;
	int process_queue[NPROC],PID[NPROC];
	int n = 0;
	for(i=q[rdytail].qprev; i != rdyhead; i=q[i].qprev){
		prio_sum += proctab[i].pprio;
		process_queue[n] = proctab[i].pprio;
		PID[n] = i;
		n++;
	}
	optr = &proctab[currpid];
	if(optr->pstate != PRFREE){
		prio_sum = prio_sum + optr->pprio;
	}
	if(prio_sum == 0){
		if(currpid == NULLPROC){
                                return OK;
                        }
                        else{
                                proc = NULLPROC;
                                 if (optr->pstate == PRCURR) {
                                        optr->pstate = PRREADY;
                                        insert(currpid,rdyhead,optr->pprio);
                                 }
                                nptr = &proctab[proc];
                                nptr->pstate = PRCURR;
                                dequeue(proc);
                                preempt = QUANTUM;
                                currpid = proc;
                                ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                                return OK;
                        }

	}
	else{
		if(prio_sum > 0){
			int random1;
			random1 = rand()%(prio_sum-1);
			i = n-1;
                        while(i>=0 && optr->pprio < process_queue[i]){
                                process_queue[i+1] = process_queue[i];
                                PID[i+1] = PID[i];
                                i--;
                        }
                        process_queue[i+1] = optr->pprio;
                        PID[i+1] = currpid;
                        n++;

			i=n-1;
			while(i>=0){
                        	if(random1 < process_queue[i]){
                                	break;
                       		}	
                        	random1 = random1 - process_queue[i];
				i--;
               		 }	
			proc = PID[i];
			nptr = &proctab[proc];
			if(nptr->pstate == PRSLEEP){
				if(PID[i-1]){
					proc = PID[i-1];
					nptr = &proctab[proc];
				}
				else{
					proc = NULLPROC;
					nptr = &proctab[proc];

				}
			}
			if(proc == currpid)
				return OK;
			else{
				if (optr->pstate == PRCURR) {
                			optr->pstate = PRREADY;
                			insert(currpid,rdyhead,optr->pprio);
                		 }
                 	nptr->pstate = PRCURR;
               		dequeue(proc);
                 	currpid = proc;
                 	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                 	return OK;
        		}
		}	
        	else{
                	return SYSERR;
        	}
	}
}

int Default(){
	register struct pentry  *p;
	int i;
	int count = 0;
	for(i=0;i< NPROC;i++){
		p = &proctab[i];
		if(p->pstate == PRREADY){
			count += p->pprio;
		}
	}
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}
	
	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}
