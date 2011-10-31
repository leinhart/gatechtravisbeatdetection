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
	float		clapTimeLast;
	float		clapTimeIn;
	float		clapModeStartTime;
	float		clapModeIdleTime; 
	int			clapperToBeginOrEnd;
	float		inThresh;
	
	//AUTOCORR
	t_float		onsetTime;
	int			onsetCountA;
	//float		autoCorrTempos[3]; //stores tempos to compare to clustering
	//float		autoCorrStyles[3]; //stores 0=straight, 1=swung for tempos
	//int			autoCorrTemposI; // to index previous
	//int			autoCorrTemposSize;
	float		onsetTimes[20];
	float		onsetIntensities[10];
	float		autoCorrelation[4000];
	float       tdOnsets[4000]; //three seconds at quantized to millis - 6 beats at 120
	int			onsetTimesSize;
	int         tdOnsetsSize;
	float		timeStart;
	int			styleA;
	float		tempoA;
	
	int			songIntervals[3][12];
	int			songIntervalsRSize;
	int			songIntervalsCSize;
	
	float		songOnsetDB[3][4000];
	float       songAutoCorrDB[3][4000];
		
}t_pdbeatclassification;

void outletSendData(t_pdbeatclassification *x, float data);
void autoCorrelation(t_pdbeatclassification *x, float in[], float out[]);
void initSongData(t_pdbeatclassification *x);
//static inline void findMatchingSong(t_pdbeatclassification *x);
void onsetsToTimeDomain(t_pdbeatclassification *x);
void autoCorrAnalyze(t_pdbeatclassification *x);
void compareOnsets(t_pdbeatclassification *x);
void combBank(t_pdbeatclassification *x);

void pdbeatclassification_float(t_pdbeatclassification *x, t_floatarg volume)
{   
	post("float in");

	//CLUSTER
	double temp;
	float interval;
	
	if(x->start == -1) {
		post("in volume = %f", volume);
		return;
	}	
	else if(x->start == 0){
		
		if (volume > x->inThresh) {		
	    x->start = 1;
		x->onsetTime = sys_getrealtime();
		//if(temp - x->lastTime > .080){ //80 millisecond threshold	
		x->timeStart = x->onsetTimes[x->onsetCountA] = (x->onsetTime)*1000.0; //millis
		x->onsetIntensities[x->onsetCountA] = volume/100.0;//= volume;
		x->onsetCountA+=1;
		post("onset time %f",x->onsetTime);
		post("onset intensity %f",volume);
		}
	}
	else if(x->start == 2) {
		//clapper state
		post("start == 2 - in volume = %f", volume);
		if (volume > x->inThresh) {	
		x->clapTimeIn = sys_getrealtime()*1000.0;
		if(x->clapTimeIn - x->clapModeIdleTime > x->clapModeStartTime) {
			
		post("CSI - CSL = %f", x->clapTimeIn - x->clapTimeLast);
		post("volume = %f", volume);
		if(x->clapTimeIn - x->clapTimeLast < 200) {
			if(x->clapperToBeginOrEnd == 0) {
				//start
				SETFLOAT(x->outstart+0, -2);
				outlet_list(x->x_outlist, &s_list, 1, (x->outstart)); //catch -2 and let user know it is listening		
				//stay in state 2 to wait for clapper again
				x->clapTimeLast = -999999;
				x->clapperToBeginOrEnd = 1;
				x->start = 0;
				x->onsetCountA = 0;
				x->oiSaveCount = 0;
			}	
			else if(x->clapperToBeginOrEnd ==1) {
				//end
				SETFLOAT(x->outstart+0, -1);
				outlet_list(x->x_outlist, &s_list, 1, (x->outstart));		
				x->clapTimeLast = -99999;
				x->clapModeStartTime = sys_getrealtime()*1000.0;
				x->clapperToBeginOrEnd = 0;
			}
				
		}
		else
			x->clapTimeLast = x->clapTimeIn;
		
		}
		}	
	}
	else if(x->start == 1) { 
		
		float temp = sys_getrealtime();
				if((temp*1000.0 - x->timeStart >= x->tdOnsetsSize - 1))
				{
					//compare to stored
					//autoCorrAnalyze(x);
					compareOnsets(x);
					x->start = 2;
					x->clapModeStartTime = sys_getrealtime()*1000.0;
					return;
				}
		
	if (volume > x->inThresh) {
			x->onsetTimes[x->onsetCountA] = temp*1000.0; //millis
		x->onsetIntensities[x->onsetCountA] = volume/10.0;// = volume;
			x->onsetCountA+=1;
			post("onset time %f",temp);
			post("onset intensity %f",volume);
			}
	}
}

void compareOnsets(t_pdbeatclassification *x){
	
	int i; 
	int j;
	int k;
	int y;
	int s;
	
	float relOnsetTimes[x->onsetTimesSize];
	float sTime = x->onsetTimes[0];
	relOnsetTimes[0] = 0;
	for(i=1;i<x->onsetTimesSize;i+=1) {
		relOnsetTimes[i] = x->onsetTimes[i] - sTime;
	}	
	
	
	//int onsetHasBeenChosen[x->songIntervalsRSize][x->onsetCountA] = {0};
	
	float sqErrors[x->songIntervalsRSize];
	
	//find closest recorded onset for each real onset
	//int minI[x->songIntervalsRSize];
	float sum[x->songIntervalsRSize];
	for(i=0; i<x->songIntervalsRSize; i+=1)
		sum[i] = 999999999.0;
	
	int stretchLow = -32;
	int stretchHigh = 32;
	int stretchSum = 32;
	int stretchInc = 2;
	
	float min = 9999999.0;
	float stretchMin = 0;
	float m;
	for(k = 0; k<x->songIntervalsRSize; k+=1) {
		s = stretchLow;
		for (y = 0; y<stretchSum; y+=1) { 
			for (i = 1; i<x->songIntervalsCSize; i+=1) {
				for (j = 0; j<x->onsetCountA; j+=1) {
					m = fabs(relOnsetTimes[j] - ((float)x->songIntervals[k][i] + i*s)); 
					if(m < min) {
						min = m;
					}
				}
				stretchMin += min;	
				min = 999999999.0;
			}
			
			if(stretchMin < sum[k]) {
				post("k = %d, stretchMin = %f", k, stretchMin);
				sum[k] = stretchMin;
			}
			stretchMin = 0;
			
			s += stretchInc;	
		}
	}
	
	if ((sum[0] > 450 && sum[1] > 450) || (fabs(sum[0] - sum[1]) ) < 200) {
	//do autocorr, send tempo	
		autoCorrAnalyze(x);
		outletSendData(x, x->tempoA);
	}	
	else {
		
		min = 99999999.0;
		int minI = -1;
		for(i = 0; i<x->songIntervalsRSize; i+=1) {
			post("sum[i] = %f", sum[i]);
			if (sum[i] < min) {
				min = sum[i];
				minI = i;
			}
		}	
	post("minI = %d", minI);
	x->chosenSongI = (float)minI;
	//return number of song
	outletSendData(x, x->chosenSongI);
		
		
	}	
}

void autoCorrAnalyze(t_pdbeatclassification *x){
	
	post("autoCorrAnalyze");
	onsetsToTimeDomain(x);
	autoCorrelation(x, x->tdOnsets, x->autoCorrelation);
	combBank(x);
	//findMatchingSong(x);

	//reset autocorr
	x->onsetCountA = 0;	
	int i;
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	post("autoCorrAnalyze Finished");
}


void onsetsToTimeDomain(t_pdbeatclassification *x){
	
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

void combBank(t_pdbeatclassification *x)
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
	float rCenter = 100; //roughly 110 bpm
	float rConst = (1/(2*(2*pow(rCenter,2))));
	post("rConst = %f", rConst);
	int maxindex = -1;
	float max = -1;
	float modCombSum;
	for(i=1; i<numCombs; i+=1){
		modCombSum = combSums[i]*((float)i*2*rConst*pow(M_E, -pow((float)i,2)*rConst));
		post("pow = %f", ((float)i*2*rConst*pow(M_E, -pow((float)i,2)*rConst))); 
		post("combSum = %f", combSums[i]);
		post("modCombSum = %f", modCombSum);
		if (modCombSum > max){
			max = modCombSum;
			maxindex = i;
		}
	}
	
	//set tempo
	x->tempoA = 60000.0/(float)(KLow - maxindex*Kdivision);
	
	post("TEMPO: %f", x->tempoA);
}


					
void outletSendData(t_pdbeatclassification *x, float data){
	
	post("outletSendData");	

		
SETFLOAT(x->outinfo+0, data);	
outlet_list(x->x_outlist, &s_list, 1, (x->outinfo));	
	
}	
	
void newDetection(t_pdbeatclassification *x)
{	
	//x->start = 0;
//	post("newDetection");
//	
//	x->onsetCountA = 0;
//	x->oiSaveCount = 0;
//	
}
	
	
void *pdbeatclassification_new(void)
{
	post("void *pdbeatclassification_new(void)");
	t_pdbeatclassification *x = (t_pdbeatclassification *)pd_new(pdbeatclassification_class);
	//x->tempo_out = outlet_new(&x->x_obj, &s_float);
	x->x_outlist = outlet_new(&x->x_obj, &s_list);
	

	x->oiSaveSize = 6;
	x->oiSaveCount = 0;
	x->onsetTimesSize = 20;
	x->tdOnsetsSize = 4000;
	x->songIntervalsRSize = 2;
	x->songIntervalsCSize = 10;
	x->clapTimeLast = -99999;
	x->clapModeIdleTime = 2000;
	x->clapModeStartTime = sys_getrealtime()*1000.0;
	x->inThresh = 8;
	x->clapperToBeginOrEnd = 0; //begin
	x->start = 2; //start listening for clapper 
	x->onsetCountA = 0;
	x->oiSaveCount = 0;

	
	//set songIntervals
	int siTemp[2][12] = {
		{0, 464, 651, 969,  1598, 1947, 2613, 3059, 3255, 3607,},  //DILIH
		{0, 446, 884, 1336, 1596, 2034, 2448, 2736, 3208, 3644,}//3644, -1  , -1  }, //VLV
		//{0, 502, 752, 1246, 1514, 1756, 1990, 2422, 2596, 2964,-1,-1}//2964, 3448, 3722} //Idioteque
	};
	
	int i;
	int j;
	
	for (i=0; i<x->songIntervalsRSize; i+=1) {
		for(j=0; j<x->songIntervalsCSize; j+=1) {
			x->songIntervals[i][j] = siTemp[i][j];
			post("SI :%d", x->songIntervals[i][j]);
		}
	}
	
	//initSongData(x);
	//x->autoCorrTemposSize = 3;
	post("NEW INITIALIZATION");
	for(i = 0; i< x->tdOnsetsSize; i++){
		x->tdOnsets[i] = 0;
	}
	
	return (void *)x;
}

void pdbeatclassification_setup(void)
{
	post("void pdbeatclassification_setup(void)");
	pdbeatclassification_class = class_new(gensym("pdbeatclassification"), (t_newmethod)pdbeatclassification_new,0 , sizeof(t_pdbeatclassification),	CLASS_DEFAULT, 0);
	
	//message handlers
	class_addfloat(pdbeatclassification_class, (t_method)pdbeatclassification_float);
	class_addmethod(pdbeatclassification_class, (t_method)newDetection, gensym("newDetection"),0);
}


//void initSongData(t_pdbeatclassification *x){
//	
//	int i;
//	int j;
//	
//	//init to 0
//	for(i = 0; i<x->songIntervalsRSize; i+=1) {
//		for(j = 0; j<x->tdOnsetsSize; j+=1) {
//			x->songOnsetDB[i][j] = 0;
//			x->songAutoCorrDB[i][j] = 0;
//		}
//	}
//	
//	//put TD onsets
//	for(i = 0; i<x->songIntervalsRSize; i+=1) {
//		for(j = 0; j<x->songIntervalsCSize; j+=1) {
//			if(x->songIntervals[i][j] != -1)
//				x->songOnsetDB[i][x->songIntervals[i][j]] = 1; 
//		}
//	}
//	
//	float tempArrIn[x->tdOnsetsSize];
//	float tempArrOut[x->tdOnsetsSize];
//	for(i = 0; i<x->songIntervalsRSize; i+=1) {
//		
//		for(j = 0; j<x->tdOnsetsSize; j+=1) {
//			tempArrIn[j] = x->songOnsetDB[i][j];
//		}
//		
//		autoCorrelation(x, tempArrIn, tempArrOut);
//		
//		for(j = 0; j<x->tdOnsetsSize; j+=1) {
//			x->songAutoCorrDB[i][j] = tempArrOut[j];
//		}
//	}
//	
//}
//
//static inline void findMatchingSong(t_pdbeatclassification *x){
//	int windowLen = 10;
//	float window[windowLen];
//	int j;
//	int i;
//	int k;
//	for(j = 0; j<windowLen/2; j+=1)
//		window[j] = ((float)j)/(windowLen*1000);
//	
//	for(j = windowLen/2; j<windowLen; j+=1)
//		window[j] = ((float)(windowLen - j))/(windowLen*1000);
//	
//	
//	//smoothing
//	for(j = windowLen/2; j<x->tdOnsetsSize-windowLen/2-1; j+=1) {
//		if (x->autoCorrelation[j] != 0) {
//			for(i = 0; i<windowLen; i+=1) {
//				x->autoCorrelation[j+i-windowLen/2] += x->autoCorrelation[j] * window[i];
//			}
//			//post("AC = %f",x->autoCorrelation[j] );
//		}
//		
//		for (k = 0; k<x->songIntervalsRSize; k+=1) {
//			if (x->songAutoCorrDB[k][j] > 0) {
//				for(i = 0; i<windowLen; i+=1) {
//					x->songAutoCorrDB[k][j+i-windowLen/2] += x->songAutoCorrDB[k][j] * window[i];
//				}
//			}
//		}
//		
//	}
//	
//	//least squared error
//	float sqErrors[x->songIntervalsRSize];
//	float sum = 0;
//	for (i = 0; i<x->songIntervalsRSize; i+=1) {
//		for (j = 0; j<x->tdOnsetsSize; j+=1) {
//			if(x->songAutoCorrDB[i][j] > 0)
//				//post("DB = %lf", x->songAutoCorrDB[i][j]);
//				
//				sum += pow(x->songAutoCorrDB[i][j] - x->autoCorrelation[j], 2);
//		}
//		post("sum = %f", sum);
//		sqErrors[i] = sum;
//		sum = 0;
//	}
//	
//	//find min of sqErrors
//	float min = 99999999;
//	int minI = -1;
//	for (i = 0; i<x->songIntervalsRSize; i+=1) {
//		post("sqErrors[i] = %f", sqErrors[i]);
//		if (sqErrors[i] < min) {
//			min = sqErrors[i];
//			minI = i;
//		}
//	}
//	
//	post("minI = %d", minI);
//	x->chosenSongI = (float)minI;
//	//return number of song
//	outletSendData(x);
//}
//
//	