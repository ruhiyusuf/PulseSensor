/*

		THIS CODE IS RELEASED WITHOUT WARRANTY OF FITNESS
		OR ANY PROMISE THAT IT WORKS, EVEN. WYSIWYG.

		YOU SHOULD HAVE RECEIVED A LICENSE FROM THE MAIN
		BRANCH OF THIS REPO. IF NOT, IT IS USING THE
		MIT FLAVOR OF LICENSE

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <pcf8591.h>

// PULSE SENSOR LEDS
#define BLINK_LED 0
//#define FADE_LED      // NEEDS THIS FEATURE ADDED

// MCP3004/8 SETTINGS
#define BASE 100
#define SPI_CHAN 0
#define PCF 120
#define PULSE_SIGNAL_MAX 1023
#define PULSE_SIGNAL_MIN 0
#define PULSE_SIGNAL_IDLE 512
#define JITTER_SIGNAL_IDLE 0
#define DATA_POINTS 100


// VARIABLES FOR GNUPLOT
char * plotDataFilename = "/home/biofeedback/Documents/pulse-sensor/PlotData.dat";
FILE * plotDataFile;
int plotData[DATA_POINTS];
// VARIABLES USED TO DETERMINE SAMPLE JITTER & TIME OUT
volatile unsigned int eventCounter, thisTime, lastTime, elapsedTime;
volatile int sumJitter,jitter;
unsigned int timeOutStart, dataRequestStart, m;
// VARIABLES USED TO DETERMINE BPM
volatile int Signal;
volatile unsigned int sampleCounter;
volatile int threshSetting,lastBeatTime,fadeLevel;
volatile int thresh = 550;      // SHOULD BE ADJUSTABLE ON COMMAND LINE
volatile int P = 512;           // set P default
volatile int T = 512;           // set T default
volatile int firstBeat = 1;     // set these to avoid noise
volatile int secondBeat = 0;    // when we get the heartbeat back
volatile int QS = 0;
volatile int rate[10];
volatile int BPM = 0;
volatile int IBI = 600;         // 600ms per beat = 100 Beats Per Minute (BPM)
volatile int Pulse = 0;         // set to 1 while Signal > Threshold
volatile int amp = 100;         // beat amplitude 1/10 of input range.
// FILE STUFF
char rawDatafilename [100];
FILE * pulseData;
struct tm *timenow;
// FUNCTION PROTOTYPES
void getPulse();
void stopTimer(void);
void initPulseSensorVariables(void);
void initJitterVariables(void);
void initPlotVariables(int idle);
void loadNewPlotData(void);
void incrementPlotData(void);


void usage()
{
   fprintf
   (stderr,
      "\n" \
      "Usage: ./gnuplotPulse ... [OPTION] ...\n" \
      "   NO OPTIONS AVAILABLE YET\n"\
      "\n"\
      "   Data file saved as\n"\
      "   /PulseData/PulsePlot_<timestamp>.dat\n"\
      "   Data format tab separated:\n"\
      "     sampleCount  Signal  BPM  IBI  Pulse  Jitter\n"\
      "\n"\
      "   Closing Program with ^C in this window will kill gnuplot\n"\
      "\n"
   );
}

void sigHandler(int sig_num){
	printf("\nkilling gnuplot and exiting\n");
	system("pkill gnuplot");
	exit(EXIT_SUCCESS);
}

void fatal(int show_usage, char *fmt, ...)
{
    char buf[128];
    va_list ap;
    char kill[20];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s\n", buf);

    if (show_usage) usage();

    fflush(stderr);
    printf("\nkilling gnuplot and exiting\n");
    system("pkill gnuplot");
    fprintf(pulseData,"#%s",fmt);
    fclose(pulseData);

    exit(EXIT_FAILURE);
}

// SAVED FOR FUTURE FEATURES
static int initOpts(int argc, char *argv[])
{
   //int i, opt;
   //while ((opt = getopt(argc, argv, ":")) != -1)
   //{
      //i = -1;
      //switch (opt)
      //{
        //case '':
        //default: /* '?' */
           //usage();
        //}
    //}
   return optind;
}


int main(int argc, char *argv[])
{
    signal(SIGINT,sigHandler);
    usage();
    //int settings = 0;
    // command line settings
    //settings = initOpts(argc, argv);
    time_t now = time(NULL);
    timenow = gmtime(&now);

    strftime(rawDatafilename, sizeof(rawDatafilename),
    "/home/biofeedback/Documents/pulse-sensor/PulsePlot_%Y-%m-%d_%H:%M:%S.dat", timenow);
    pulseData = fopen(rawDatafilename, "w+");
    fprintf(pulseData,"#Running with delayMicroseconds Timer\n");
    fprintf(pulseData,"#sampleCount\tSignal\tBPM\tIBI\tjitter\n");

	// open gnupolot with a FIFO
    printf("\nOpening gnuplotPipe\n");
	FILE * gnuplotPipe = popen("gnuplot -persistent", "w");
	// tell gnuplot to read a config file
	fprintf(gnuplotPipe,"load 'Pulse_Plot_Config.gnu'\n");
    printf("Starting wiringPi and initializeing variables\n");

    wiringPiSetup();                //use the wiringPi pin numbers
    // mcp3004Setup(BASE,SPI_CHAN);    // setup the mcp3004 library
		pcf8591Setup(PCF, 0x48);
//    pinMode(BLINK_LED, OUTPUT); digitalWrite(BLINK_LED,LOW);
    initPulseSensorVariables();     // initilaize Pulse Sensor beat finder
    initPlotVariables(PULSE_SIGNAL_IDLE);  // initialize plot values
    printf("here we go\n");

    while(1)
    {
        delayMicroseconds(1800);    // DELAY TIME NEEDS TO BE ATOMATED
		getPulse();
        fprintf(pulseData,"%d\t%d\t%d\t%d\t%d\n",
        sampleCounter,Signal,BPM,IBI,jitter
        );
        //printf("%d\t%d\t%d\t%d\t%d\n",
        //sampleCounter,Signal,IBI,BPM,jitter
        //);

        if(sampleCounter%40 == 0){
            incrementPlotData();
            loadNewPlotData();
            fprintf(gnuplotPipe,"plot \"PlotData.dat\" with lines lt rgb \"red\"\n");
            fflush(gnuplotPipe);
            //int writeTime = micros() - timeOutStart;
            //if(writeTime > 2000){
                //printf("%d\n",writeTime);
            //}
        }
    }

    return 0;

}//int main(int argc, char *argv[])


void initPulseSensorVariables(void){
    for (int i = 0; i < 10; ++i) {
        rate[i] = 0;
    }
    QS = 0;
    BPM = 0;
    IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
    Pulse = 0;
    sampleCounter = 0;
    lastBeatTime = 0;
    P = 512;                    // peak at 1/2 the input range of 0..1023
    T = 512;                    // trough at 1/2 the input range.
    threshSetting = 550;        // used to seed and reset the thresh variable
    thresh = 550;     // threshold a little above the trough
    amp = 100;                  // beat amplitude 1/10 of input range.
    firstBeat = 1;           // looking for the first beat
    secondBeat = 0;         // not yet looking for the second beat in a row
    lastTime = micros();
}

void getPulse(){     //int sig_num){

    thisTime = micros();
    Signal = analogRead(PCF + 0);
    // Signal = analogRead(BASE);
    elapsedTime = thisTime - lastTime;
    lastTime = thisTime;
    jitter = elapsedTime - 2000;
    sumJitter += jitter;


  sampleCounter += 2;         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise



}

void initPlotVariables(int idle){
    for(int i=0; i<DATA_POINTS; i++){
		plotData[i] = idle;
	}
    loadNewPlotData();
}

void loadNewPlotData(void){
	plotDataFile = fopen(plotDataFilename,"w+");
	for(int i=0; i<DATA_POINTS; i++){
		fprintf(plotDataFile,"%d\t%d\n",i+1,plotData[i]);
	}
	fflush(plotDataFile);
	fclose(plotDataFile);
}

void incrementPlotData(void){
	for(int i=0; i<DATA_POINTS-1; i++){
		plotData[i] = plotData[i+1];
	}
	plotData[DATA_POINTS-1] = Signal;
}
