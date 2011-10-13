/*
 *  pdBeatDetection.c
 *  pdBeatDetection
 *
 *  Created by Mason Bretan on 9/28/11.
 *  Copyright 2011 UCSD. All rights reserved.
 *
 */

#include "m_pd.h"
#include <math.h>

static t_class *pdbeatdetection_class;
typedef struct _pdbeatdetection{
	//BOTH
	t_object	x_obj;
	//t_outlet	*tempo_out;
	t_float		tempoA;
	t_float		tempoC;
	t_float		tempoBoth;
	t_outlet    *x_outlist;
	t_atom outinfo[2];
	int			start;  //boolean for if it is the first onset

	//CLUSTER
	t_float		lastTime;
	t_float		lastTempo; 
	float		currentSetTempo;
	int			onsetCountC;
	float		onsetIntervals[11];
	float		intensity[11];
	int			clusterMissFires;
	//AUTOCORR
	t_float		onsetTime;
	int			onsetCountA;
	float		autoCorrTempos[3]; //stores tempos to compare to clustering
	float		autoCorrStyles[3]; //stores 0=straight, 1=swung for tempos
	int			autoCorrTemposI; // to index previous
	int			autoCorrTemposSize;
	float		onsetTimes[100];
	float		onsetIntensities[100];
	float		autoCorrelation[1500];
	float       tdOnsets[1500]; //three seconds at quantized to millis - 6 beats at 120
	int			onsetTimesSize;
	int         tdOnsetsSize;
	float		timeStart;
	float		styleA;
	
	//start: 
	//		-1 = ignore floats, 
	//		0 = waiting for first float,
	//		1 = getting floats 
		
	
}t_pdbeatdetection;

//BOTH
static inline void compareClusterToAuto(t_pdbeatdetection *x);
static inline void outletSendData(t_pdbeatdetection *x);

//AUTOCORR
static inline void combBank(t_pdbeatdetection *x);
static inline void autoCorrelation(t_pdbeatdetection *x);
static inline void onsetsToTimeDomain(t_pdbeatdetection *x);
static inline void autoCorrAnalyze(t_pdbeatdetection *x);

//CLUSTER
static inline int isSimilar(float x, float y);
static inline int qtClustering(t_pdbeatdetection *x);
static inline void setTempo(t_pdbeatdetection *x, float interval);
static inline int withinError(float min, float test, float interval);
static inline int isSimilarTempo(float x, float y);
static inline void updateArrays(t_pdbeatdetection *x, float interval, float intensity);


//BOTH
void pdbeatdetection_float(t_pdbeatdetection *x, t_floatarg volume)
{   
	
	//CLUSTER
	double temp;
	float interval;
	
	if(x->start == -1)
		return;
	else if(x->start == 0){
		//CLUSTER
		x->lastTime = sys_getrealtime();
		
		//AUTOCORR
	    x->start = 1;
		x->onsetTime = sys_getrealtime();
		//if(temp - x->lastTime > .080){ //80 millisecond threshold	
		x->timeStart = x->onsetTimes[x->onsetCountA] = (x->onsetTime)*1000.0; //millis
		x->onsetIntensities[x->onsetCountA] = volume/1000.0;
		x->onsetCountA+=1;
		post("onset time %f",x->onsetTime);
	}
	else{ 
		
		
	//CLUSTER	
		temp = sys_getrealtime();
		interval = (temp - (x->lastTime))*1000;  //interval in milliseconds
		
		
		if(temp - x->lastTime > .080){ //80 millisecond threshold	
			//post("interval %f",interval);
			
			updateArrays(x, interval, volume);
			x->lastTime = temp;
			
			if (x->onsetCountC>=10) {   //clustering looks at only the last 11 onsets
				if(qtClustering(x) == 1){
					
					//qt clustering method found a good tempo so at this point compare to autocorrelation
					
				}else{
					
					//qtClustering didn't find a good tempo so do something else
				}
			}
			x->onsetCountC++;
		}

	//AUTOCORR
		
		if (x->onsetCountA < x->onsetTimesSize - 1){
				if(temp*1000.0 - x->timeStart >= x->tdOnsetsSize - 1)
				{
					autoCorrAnalyze(x);
					return;
				}
			//if(temp - x->lastTime > .080){ //80 millisecond threshold	
			x->onsetTimes[x->onsetCountA] = temp*1000.0; //millis
			x->onsetIntensities[x->onsetCountA] = volume/1000.0;
			x->onsetCountA+=1;
			post("onset time %f",temp);
			if(x->onsetCountA >= x->onsetTimesSize -1)
				autoCorrAnalyze(x);
			}
		
	}
}

//AUTOCORR
static inline void autoCorrAnalyze(t_pdbeatdetection *x){
	
	post("autoCorrAnalyze");
	onsetsToTimeDomain(x);
	autoCorrelation(x);
	combBank(x);
	
	//outlet_float(x->tempo_out, x->tempo);
	post("AUTOCORR TEMP:  %lf", x->tempoA);
	post("AUTOCORR STYLE: %d", x->styleA);
	
	//log temp and style
	x->autoCorrStyles[x->autoCorrTemposI] = x->styleA;
	x->autoCorrTempos[x->autoCorrTemposI] = x->tempoA;
	x->autoCorrTemposI += 1;
	if(x->autoCorrTemposI >= x->autoCorrTemposSize)
		x->autoCorrTemposI -= x->autoCorrTemposSize;
	
	if(x->start == 1) //maybe to fix threading issues
	x->start = 0;
	
	
	//reset autocorr
	x->onsetCountA = 0;	
	int i;
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	
}

//AUTOCORR
static inline void onsetsToTimeDomain(t_pdbeatdetection *x){
	
	post("onsetToTimeDomain");
	int i;
	int oi;
	
	x->tdOnsets[0] = x->onsetIntensities[0];   // just to avoid possible floating-point subtraction weirdness
	for (i=1;i < x->onsetCountA;i+=1){
		//truncate for now
		oi = (int)(x->onsetTimes[i] - x->timeStart);
		x->tdOnsets[oi] = x->onsetIntensities[i];
	}
	//post("ONSETSTOTIMEDOMAIN FINISHED");
}

//AUTOCORR
static inline void autoCorrelation(t_pdbeatdetection *x){
	
	post("autoCorrelation");
	
	int i;
	int j;
	int os = x->tdOnsetsSize;
	float sum = 0;
	
	for(i = 0; i< os; i+=1){
		//lag = i;
		for(j = 0; j < os - i; j+=1){
			sum += x->tdOnsets[j] * x->tdOnsets[j+i];
		}
		//to remove bias
		sum *= (1 +i);
		sum = pow(sum, 2);
		
		x->autoCorrelation[i] = sum;
		if(sum>.5)
			post("AUTOCORRELATION:  %f",sum );
		sum = 0;
	}	
	//post("AUTOCORRELATION:  %f \n %f \n %f",x->autoCorrelation[1], x->autoCorrelation[2], x->autoCorrelation[3] );
}

//AUTOCORR
static inline void combBank(t_pdbeatdetection *x)
{
	
	post("combBank");
	//bootleg td comb filter
	
	//300 - 1000
	int Kdivision = 5;
	int lowTempo = 60;
	int highTempo = 200;
	int KLow = 60000/lowTempo;
	int KHigh = 60000/highTempo;
	int numCombs = (KLow-KHigh)/Kdivision;
	float combSums[numCombs];
	float window[7] = {.5,.75,1,1,1,.75,.5};
	float numSums;
	
	float sum = 0;
	int K = KLow;
	int Ki = 0;
	int i;
	int j;
	int w;
	for(i=0; i<numCombs; i+=1){
		numSums = x->tdOnsetsSize/K;
		
		//second half of window
		for(j=0; j<4; j+=1){
			sum += (x->autoCorrelation[j]*window[j+3]);
		}
		
		Ki += K;
		//normally sized
		for(; Ki < x->tdOnsetsSize-4; Ki+=K){
			for(w = -3; w<4; w+=1){
				sum += (x->autoCorrelation[Ki+w] * window[w+3]);
			}						   
		}
		
		//TODO: compute partial window that may clash at end
		
		combSums[i] = sum/numSums;
		//post("COMBSUMS: %f \n", combSums[i]);
	    Ki = 0;
		K -= Kdivision;
		sum = 0;
	}
	
	//get max
	int maxindex = -1;
	float max = -1;
	for(i=0; i<numCombs; i+=1){
		if (combSums[i] > max){
			max = combSums[i];
			maxindex = i;
		}
	}
	
	//see which is higher - 2nd or 3rd harmonic (straight or swung)
	float second = 0;
	float third = 0;
	if (i*2 < numCombs)
		second = combSums[i*2];
		
	if (i*3 < numCombs)
		third = combSums[i*3];
	
	if(second >= third)
		x->styleA = 0;   //straight
	else x->styleA = 1;  //swung
	
	
	//set tempo
	x->tempoA = 60000.0/(float)(KLow - maxindex*Kdivision);
	
	//post("COMBBANK FINISHED");
	
}

//BOTH
void newDetection(t_pdbeatdetection *x)
{	
	x->start = 0;
	post("newDetection");
	//AUTOCORR
	x->onsetCountA = 0;
	x->autoCorrTemposI = 0;
	
	//CLUSTER
	int i;
	for(i = 0; i< x->onsetCountC; i++){
		x->onsetIntervals[i] = 0;
	}
	
	x->onsetCountC = 0;
	x->currentSetTempo = -1;
	x->lastTempo = -1;
	
}

//CLUSTER
static inline void updateArrays(t_pdbeatdetection *x, float interval, float intensity){
	
	// updating the arrays with the most recent values in array[10] position
	// hopefully this will prevent pd from crashing
	post("updateArrays");
	int i;
	for(i = 0; i <=9; i ++){
		x->onsetIntervals[i] = x->onsetIntervals[i+1];
		x->intensity[i] = x->intensity[i+1];
	}
	x->onsetIntervals[10] = interval;
	x->intensity[10] = intensity;
}

//CLUSTER
static inline int qtClustering(t_pdbeatdetection *x)
{
	post("qtClustering");
	int range = 10;  //looking at 11 previous onsets
	
	int clusterType[11] = {0};
	int clusterCount=0;
	int inCluster[11] = {0};
	float clusters[11] = {0};
	int typeCount[11] = {0};
	int i,j;
	
	for(i = 0; i < range; i++){
		
		for(j = i+1; j<=range;j++){
			if(inCluster[j] == 0){
				if(isSimilar(x->onsetIntervals[i], x->onsetIntervals[j+i]) == 1){					
					if(inCluster[i] == 1){
						clusterType[j] = clusterType[i];
						inCluster[j] = 1;
					}else{
						clusterCount++;
						clusterType[i] = clusterCount;
						clusterType[j] = clusterCount;
						inCluster[i]= 1;
						inCluster[j] = 1;
					}
				}
			}
		}
	}
	
	for(i = 0; i <= range; i++){
		typeCount[i] = 0;
		if(inCluster[i] == 0){
			clusterCount++;
			clusterType[i] = clusterCount;
			inCluster[i] = 1;
		}
		//post("cluster type   %d", clusterType[i]);
	}
	
	
	//analysis
	for(i = 0; i<=range; i++){		
		for(j = 0; j<clusterCount; j++){
			if(clusterType[i] == j+1){
				clusters[j] += x->onsetIntervals[i];
				typeCount[j]++;
				break;
			}
		}
	}
	
	float min = 100000000.;
	int minPos;
	float max = -1;
	int maxPos;	
	for(i = 0; i<clusterCount;i++){
		clusters[i] /= typeCount[i];
		//post("cluster avg   %lf     %d", clusters[i], typeCount[i]);
		if(clusters[i] < min){
			min =clusters[i];
			minPos = i;
		}
		if(clusters[i] > max){
			max = clusters[i];
			maxPos = i;
		}
	}
	
	
	
	int tatumFit = 0;
	int errorCount = 0;
	int errors[11] = {0};
	int adjust = 0;
	if(clusterCount < 1){
		setTempo(x,clusters[maxPos]);
		return 1;
		
	}else{
		
		for(i = 0; i <clusterCount; i++){						
			
			if(withinError(clusters[minPos], clusters[i], 1)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 1.5)){
				tatumFit++;
				if(i == maxPos)
					adjust = 1;
			}else if(withinError(clusters[minPos], clusters[i], 2)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 3)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 4)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 5)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 6)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 7)){
				tatumFit++;
			}else if(withinError(clusters[minPos], clusters[i], 8)){
				tatumFit++;
			}else{
				errors[errorCount] = i;
				errorCount++;
			}
		}
		
		
		if(errorCount > 0){
			for(i=0; i<errorCount; i++){
				if(withinError(clusters[errors[i]], clusters[maxPos],3)){
					tatumFit++;
				}else
					if(withinError(clusters[errors[i]], clusters[maxPos],3)){
						tatumFit++;
					}
			}
		}
		
		
		float temp = 0;
		if(tatumFit >= clusterCount){   //currently all of the clusters need to be related for it to return a tempo
			if(adjust == 0){
				setTempo(x ,clusters[maxPos]); //i'm assuming that the cluster with the greatest interval is the quarter note
			}else{
				temp = clusters[maxPos] *(2.0/3); //this makes an adjustment if the longest interval can't be a quarter note in relation to the other cluster values
				//post("adjust  %lf", temp);
				setTempo(x,temp);
			}
			x->clusterMissFires = 0;
			return 1;
		}else{
			//post("%d    %d", clusterCount, tatumFit);
			x->clusterMissFires++;
			if(x->clusterMissFires > 6){  //currently if clustering fails 6 times in a row then it looks at the ACF
				return 0;
			}else{
				return 1;
			}
		}
	}	
}


//CLUSTER
static inline int withinError(float min, float test, float interval)
{
	//post("withinError");
	if(test >= min*interval - (min*interval)*.12 && test<=min*interval + (min*interval)*.12){
		return 1;
	}else{
		return 0;
	}
	
}

//CLUSTER
static inline void setTempo(t_pdbeatdetection *x, float interval)
{
	post("setTempo");
	//setting the tempo given what is the assumed quarter note interval
	x->tempoC = (1/(interval / 1000)) * 60;
	post("tempos   %lf     %lf", x->tempoC, x->lastTempo);
	float t = 0.10; //tolerance
	//if(x->tempoC <= x->currentSetTempo + x->currentSetTempo*.08 && x->tempoC >= x->currentSetTempo - x->currentSetTempo*.08){
	if(
	   (x->tempoC <= x->lastTempo + x->lastTempo*t && x->tempoC >=x->lastTempo - x->lastTempo*t)
	   ||
	   (x->tempoC <= x->lastTempo/2.0 + x->lastTempo/2.0*t && x->tempoC >=x->lastTempo/2.0 - x->lastTempo/2.0*t)
	   ||
	   (x->tempoC <= x->lastTempo*2.0 + x->lastTempo*2.0*t && x->tempoC >=x->lastTempo*2.0 - x->lastTempo*2.0*t)
	   ){
		//outlet_float(x->tempo_out, x->tempo);
		compareClusterToAuto(x);
		//x->currentSetTempo = x->tempoC;
		x->lastTempo = x->tempoC;
	}
	x->lastTempo = x->tempoC;
}

//CLUSTER
static inline int isSimilar(float x, float y)
{
	//post("isSimiliar");
	if(x > y){
		
		if( y >= x - (x*.13) && y <= x+x*.13){
			return 1;
		}else {
			return 0;
		}
		
	}else{
		
		if( x >= y - (y*.13) && x <= y+y*.13){
			return 1;
		}else {
			return 0;
		}		
	}
}

//CLUSTER
static inline int isSimilarTempo(float x, float y)
{
	//post("isSimiliarTempo");
	if(x > y){
		
		if( y >= x - (x*.08) && y <= x+x*.08){
			return 1;
		}else {
			return 0;
		}
		
	}else{
		
		if( x >= y - (y*.08) && x <= y+y*.08){
			return 1;
		}else {
			return 0;
		}		
	}
}

//BOTH
static inline void compareClusterToAuto(t_pdbeatdetection *x)
{
	post("compareClusterToAuto");
	//compare tempo C to autoCorrTempos[1:3] and their lower and upper octaves 
	float tempoDiffMat[x->autoCorrTemposSize][3]; //columns correspond to [lower octave; unison; upper]
	int i;
	for(i = 0; i < x->autoCorrTemposSize; i+=1){
		
		tempoDiffMat[i][0] = fabs(x->tempoC - x->autoCorrTempos[i]/2.0);
		tempoDiffMat[i][1] = fabs(x->tempoC - x->autoCorrTempos[i]);
		tempoDiffMat[i][2] = fabs(x->tempoC - x->autoCorrTempos[i]*2.0);

	}
	
	float min = 1000000.0;
	int minI;
	int minJ;
	int j;
	for(i = 0; i < x->autoCorrTemposSize; i+=1){
		for(j = 0; j < 3; j+=1){
			if(fabs(tempoDiffMat[i][j]) < min){
				min = fabs(tempoDiffMat[i][j]);
				minI = i;
				minJ = j;
				
			}
		}
	}
	
	// tolerance of +-8 bpm
	if (min < 8.0){
		//choose the average of whatever octave of each is between 75-150 bpm
		x->start = -1;
		if (minJ == 1){ //unison
			x->tempoBoth = (x->tempoC + x->autoCorrTempos[minI]) / 2.0; 	
			//outlet_float(x->tempo_out, x->tempoBoth);
			outletSendData(x);
		}
		else if(minJ == 0){ //lower
			if (x->autoCorrTempos[minI] > 75.0){
				x->tempoBoth = (x->tempoC*2.0 + x->autoCorrTempos[minI]) / 2.0; 	
				//outlet_float(x->tempo_out, x->tempoBoth);
				outletSendData(x);
			}
			else{
				x->tempoBoth = (x->tempoC + x->autoCorrTempos[minI]/2.0) / 2.0; 	
				//outlet_float(x->tempo_out, x->tempoBoth);
				outletSendData(x);
			}
		}
		else if(minJ == 2){ //upper
			if (x->tempoC < 150.0){	
				x->tempoBoth = (x->tempoC + x->autoCorrTempos[minI]*2.0) / 2.0; 	
				//outlet_float(x->tempo_out, x->tempoBoth);
				outletSendData(x);
			}
			else{
				x->tempoBoth = (x->tempoC/2.0 + x->autoCorrTempos[minI]) / 2.0; 	
				//outlet_float(x->tempo_out, x->tempoBoth);
				outletSendData(x);
			}
		}
	}
	else {
		post("Cluster and AutoCorr tempos not resolved");	
	}
	
}


static inline void outletSendData(t_pdbeatdetection *x){
	
	post("outletSendData");	
//find most prominent style in last 3 autocorrs	
	int i;
	float styleOut;
	float sum = 0;
	for(i=0; i<x->autoCorrTemposSize; i+=1) {
		sum += x->autoCorrStyles[i];
	}
	sum /= x->autoCorrTemposSize;
	
	if(sum<0.5)
		styleOut = 0;
	else styleOut = 1;
	
SETFLOAT(x->outinfo+0, x->tempoBoth);
SETFLOAT(x->outinfo+1, styleOut);	
outlet_list(x->x_outlist, &s_list, 2, (x->outinfo));	
	
}	
	
void *pdbeatdetection_new(void)
{
	t_pdbeatdetection *x = (t_pdbeatdetection *)pd_new(pdbeatdetection_class);
	//x->tempo_out = outlet_new(&x->x_obj, &s_float);
	x->x_outlist = outlet_new(&x->x_obj, &s_list);
	
	//AUTOCORR
	x->tdOnsetsSize = 1500;  
	x->onsetTimesSize = 11;
	x->autoCorrTemposSize = 3;
	x->start = -1;
	post("NEW INITIALIZATION");
	int i;
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	
	//CLUSTER
	x->lastTempo = -1;
	x->currentSetTempo=-1;
	x->clusterMissFires = 0;
	
	return (void *)x;
}

void pdbeatdetection_setup(void)
{
	pdbeatdetection_class = class_new(gensym("pdbeatdetection"), (t_newmethod)pdbeatdetection_new,0 , sizeof(t_pdbeatdetection),	CLASS_DEFAULT, 0);
	
	//message handlers
	class_addfloat(pdbeatdetection_class, (t_method)pdbeatdetection_float);
	class_addmethod(pdbeatdetection_class, (t_method)newDetection, gensym("newDetection"),0);
}

