#include "m_pd.h"
#include <math.h>

static t_class *pdbeatclassification_class;
typedef struct _pdbeatclassification{
	//BOTH
	t_object	x_obj;
	//t_outlet	*tempo_out;
	t_float		chosenSongI;
	t_outlet    *x_outlist;
	t_atom		outinfo[1];
	t_atom		outstart[1];
	int			start;  //boolean for if it is the first onset
	float		oiSave[6];
	int			oiSaveCount;
	int			oiSaveSize;
	float		oiSaveAvg;

	//AUTOCORR
	t_float		onsetTime;
	int			onsetCountA;
	//float		autoCorrTempos[3]; //stores tempos to compare to clustering
	//float		autoCorrStyles[3]; //stores 0=straight, 1=swung for tempos
	//int			autoCorrTemposI; // to index previous
	//int			autoCorrTemposSize;
	float		onsetTimes[10];
	float		onsetIntensities[10];
	float		autoCorrelation[4000];
	float       tdOnsets[4000]; //three seconds at quantized to millis - 6 beats at 120
	int			onsetTimesSize;
	int         tdOnsetsSize;
	float		timeStart;
	int			styleA;
	
	int			songIntervals[3][12];
	int			songIntervalsRSize;
	int			songIntervalsCSize;
	
	float		songOnsetDB[3][4000];
	float       songAutoCorrDB[3][4000];
	
	//start: 
	//		-1 = ignore floats, 
	//		0 = waiting for first float,
	//		1 = getting floats 
	//		2 = got tempo, waiting to send bang
		
}t_pdbeatclassification;

static inline void outletSendData(t_pdbeatclassification *x);
void autoCorrelation(t_pdbeatclassification *x, float in[], float out[]);
void initSongData(t_pdbeatclassification *x);
static inline void findMatchingSong(t_pdbeatclassification *x);
static inline void onsetsToTimeDomain(t_pdbeatclassification *x);
static inline void autoCorrAnalyze(t_pdbeatclassification *x);
static inline void compareOnsets(t_pdbeatclassification *x);

void pdbeatclassification_float(t_pdbeatclassification *x, t_floatarg volume)
{   
	
	//CLUSTER
	double temp;
	float interval;
	
	if(x->start == -1)
		return;
	else if(x->start == 0){
		
	    x->start = 1;
		x->onsetTime = sys_getrealtime();
		//if(temp - x->lastTime > .080){ //80 millisecond threshold	
		x->timeStart = x->onsetTimes[x->onsetCountA] = (x->onsetTime)*1000.0; //millis
		x->onsetIntensities[x->onsetCountA] = volume/100.0;//= volume;
		x->onsetCountA+=1;
		post("onset time %f",x->onsetTime);
		post("onset intensity %f",volume);
	}
	else if(x->start == 2) {
		
		if (volume > x->oiSaveAvg) {
			SETFLOAT(x->outstart+0, -1);
			outlet_list(x->x_outlist, &s_list, 1, (x->outstart));		
			x->start = -1;
		}
			
		
	}
	else if(x->start == 1) { 
		
		float temp = sys_getrealtime();
		
		x->oiSave[x->oiSaveCount] = volume;
		if(x->oiSaveCount < x->oiSaveSize - 1)
			x->oiSaveCount += 1;
		else x->oiSaveCount = 0;
		
				if((temp*1000.0 - x->timeStart >= x->tdOnsetsSize - 1) || x->onsetCountA > x->onsetTimesSize-1)
				{
					//compare to stored
					//autoCorrAnalyze(x);
					compareOnsets(x);
					x->start = 2;
					return;
				}
			
			x->onsetTimes[x->onsetCountA] = temp*1000.0; //millis
		x->onsetIntensities[x->onsetCountA] = volume/10.0;// = volume;
			x->onsetCountA+=1;
			post("onset time %f",temp);
			post("onset intensity %f",volume);
			}
}

static inline void compareOnsets(t_pdbeatclassification *x){
	
	int i; 
	int j;
	
	float relOnsetTimes[x->onsetTimesSize];
	float sTime = x->onsetTimes[0];
	relOnsetTimes[0] = 0;
	for(i=1;i<x->onsetTimesSize;i++) {
		relOnsetTimes[i] = x->onsetTimes[i] - sTime;
	}	
	
	float sqErrors[x->songIntervalsRSize];
	float sum = 0;
	for (i = 0; i<x->songIntervalsRSize; i+=1) {
		for (j = 0; j<x->onsetTimesSize; j+=1) {
				//post("DB = %lf", x->songAutoCorrDB[i][j]);
				
				sum += pow(x->songIntervals[i][j] - relOnsetTimes[j], 2);
		}
		post("sum = %f", sum);
		sqErrors[i] = sum;
		sum = 0;
	}
	
	float min = 99999999;
	int minI = -1;
	for(i = 0; i<x->songIntervalsRSize; i+=1) {
		post("sqErrors[i] = %f", sqErrors[i]);
		if (sqErrors[i] < min) {
			min = sqErrors[i];
			minI = i;
		}
	}
	
	post("minI = %d", minI);
	x->chosenSongI = (float)minI;
	//return number of song
	outletSendData(x);

}

static inline void autoCorrAnalyze(t_pdbeatclassification *x){
	
	post("autoCorrAnalyze");
	onsetsToTimeDomain(x);
	autoCorrelation(x, x->tdOnsets, x->autoCorrelation);
		
	findMatchingSong(x);

	//reset autocorr
	x->onsetCountA = 0;	
	int i;
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	post("autoCorrAnalyze Finished");
}


static inline void onsetsToTimeDomain(t_pdbeatclassification *x){
	
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
	
	

void autoCorrelation(t_pdbeatclassification *x, float in[], float out[]){
	
	post("autoCorrelation");
	
	int i;
	int j;
	int os = x->tdOnsetsSize;
	float sum = 0;
	
	for(i = 0; i< os; i+=1){
		//lag = i;
		for(j = 0; j < os - i; j+=1){
			sum += in[j] * in[j+i];
		}
		//to remove bias
		//sum *= (1 +i);
		//sum = pow(sum, 2);
		
		out[i] = sum;
		sum = 0;
	}	
	//post("AUTOCORRELATION:  %f \n %f \n %f",x->autoCorrelation[1], x->autoCorrelation[2], x->autoCorrelation[3] );
}

	void initSongData(t_pdbeatclassification *x){
		
		int i;
		int j;
		
		//init to 0
		for(i = 0; i<x->songIntervalsRSize; i+=1) {
			for(j = 0; j<x->tdOnsetsSize; j+=1) {
					x->songOnsetDB[i][j] = 0;
					x->songAutoCorrDB[i][j] = 0;
			}
		}
		
		//put TD onsets
		for(i = 0; i<x->songIntervalsRSize; i+=1) {
			for(j = 0; j<x->songIntervalsCSize; j+=1) {
				if(x->songIntervals[i][j] != -1)
					x->songOnsetDB[i][x->songIntervals[i][j]] = 1; 
			}
		}
		
		float tempArrIn[x->tdOnsetsSize];
		float tempArrOut[x->tdOnsetsSize];
		for(i = 0; i<x->songIntervalsRSize; i+=1) {
		
			for(j = 0; j<x->tdOnsetsSize; j+=1) {
				tempArrIn[j] = x->songOnsetDB[i][j];
			}
			
			autoCorrelation(x, tempArrIn, tempArrOut);
			
			for(j = 0; j<x->tdOnsetsSize; j+=1) {
				x->songAutoCorrDB[i][j] = tempArrOut[j];
			}
		}

	}
	
	static inline void findMatchingSong(t_pdbeatclassification *x){
		int windowLen = 10;
		float window[windowLen];
		int j;
		int i;
		int k;
		for(j = 0; j<windowLen/2; j+=1)
			window[j] = ((float)j)/(windowLen*1000);
		
		for(j = windowLen/2; j<windowLen; j+=1)
			window[j] = ((float)(windowLen - j))/(windowLen*1000);
		
		
		//smoothing
		for(j = windowLen/2; j<x->tdOnsetsSize-windowLen/2-1; j+=1) {
			if (x->autoCorrelation[j] != 0) {
				for(i = 0; i<windowLen; i+=1) {
					x->autoCorrelation[j+i-windowLen/2] += x->autoCorrelation[j] * window[i];
				}
				//post("AC = %f",x->autoCorrelation[j] );
			}
			
			for (k = 0; k<x->songIntervalsRSize; k+=1) {
				if (x->songAutoCorrDB[k][j] > 0) {
					for(i = 0; i<windowLen; i+=1) {
						x->songAutoCorrDB[k][j+i-windowLen/2] += x->songAutoCorrDB[k][j] * window[i];
					}
				}
			}
			
		}
		
		//least squared error
		float sqErrors[x->songIntervalsRSize];
		float sum = 0;
		for (i = 0; i<x->songIntervalsRSize; i+=1) {
			for (j = 0; j<x->tdOnsetsSize; j+=1) {
				if(x->songAutoCorrDB[i][j] > 0)
				//post("DB = %lf", x->songAutoCorrDB[i][j]);
				
				sum += pow(x->songAutoCorrDB[i][j] - x->autoCorrelation[j], 2);
			}
			post("sum = %f", sum);
			sqErrors[i] = sum;
			sum = 0;
		}
		
		//find min of sqErrors
		float min = 99999999;
		int minI = -1;
		for (i = 0; i<x->songIntervalsRSize; i+=1) {
			post("sqErrors[i] = %f", sqErrors[i]);
			if (sqErrors[i] < min) {
				min = sqErrors[i];
				minI = i;
			}
		}
		
		post("minI = %d", minI);
		x->chosenSongI = (float)minI;
		//return number of song
		outletSendData(x);
	}
				
static inline void outletSendData(t_pdbeatclassification *x){
	
	post("outletSendData");	

		
SETFLOAT(x->outinfo+0, x->chosenSongI);	
outlet_list(x->x_outlist, &s_list, 1, (x->outinfo));	
	
}	
	
void newDetection(t_pdbeatclassification *x)
{	
	x->start = 0;
	post("newDetection");
	
	x->onsetCountA = 0;
	x->oiSaveCount = 0;
	
}
	
	
void *pdbeatclassification_new(void)
{
	t_pdbeatclassification *x = (t_pdbeatclassification *)pd_new(pdbeatclassification_class);
	//x->tempo_out = outlet_new(&x->x_obj, &s_float);
	x->x_outlist = outlet_new(&x->x_obj, &s_list);
	

	x->oiSaveSize = 6;
	x->oiSaveCount = 0;
	x->onsetTimesSize = 10;
	x->tdOnsetsSize = 4000;
	x->songIntervalsRSize = 3;
	x->songIntervalsCSize = 12;
	
	//set songIntervals
	int siTemp[3][12] = {
		{0, 464, 651, 969,  1598, 1947, 2613, 3059, 3255, 3607,-1,-1},  //DILIH
		{0, 446, 884, 1336, 1596, 2034, 2448, 2736, 3208, 3644,-1,-1},//3644, -1  , -1  }, //VLV
		{0, 502, 752, 1246, 1514, 1756, 1990, 2422, 2596, 2964,-1,-1}//2964, 3448, 3722} //Idioteque
	};
	
	int i;
	int j;
	
	for (i=0; i<x->songIntervalsRSize; i+=1) {
		for(j=0; j<x->songIntervalsCSize; j+=1) {
			x->songIntervals[i][j] = siTemp[i][j];
			post("SI :%d", x->songIntervals[i][j]);
		}
	}
	
	initSongData(x);
	//x->autoCorrTemposSize = 3;
	x->start = -1;
	post("NEW INITIALIZATION");
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	
	return (void *)x;
}

void pdbeatclassification_setup(void)
{
	pdbeatclassification_class = class_new(gensym("pdbeatclassification"), (t_newmethod)pdbeatclassification_new,0 , sizeof(t_pdbeatclassification),	CLASS_DEFAULT, 0);
	
	//message handlers
	class_addfloat(pdbeatclassification_class, (t_method)pdbeatclassification_float);
	class_addmethod(pdbeatclassification_class, (t_method)newDetection, gensym("newDetection"),0);
}

	