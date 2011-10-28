package bd.travisbeatdetection;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.Math;
import android.app.Activity;
import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.util.Log;
import android.view.KeyEvent;


public class TravisBCAudioPlayback {

	class songData {
		public final String fileNumber;
		public final float tempo;
		public final long bias;
		public songData (String fn, float t, long b){
			this.fileNumber = fn;
			this.tempo = t;
			this.bias = b;
		}
	}
	
	private static final String TAG = "TravisAudioPlayback";
	private List<songData> songs;
	private songData chosenSong;
	private MediaPlayer chosenSongPlayer;
	private MediaPlayer clickPlayer;
	private ArrayList<Long> beats;
	private List<Long> classTempos;
	private List<Long> classBiases;
	private int classIndex;
	static final String BASEPATH = "mnt/sdcard/Music/TravisDatabase/";
	static final String BCPATH = "mnt/sdcard/Music/TravisDatabase/BCSongs/";
	static final String BASEPATHBEATS = "mnt/sdcard/Music/TravisDatabase/Beats/";
	final Activity owner;
	private int duration;
	Context context;
	Thread clickThread;
	
	public TravisBCAudioPlayback(Activity a){
		owner = a;
		context = a.getApplicationContext();
		songs = Arrays.asList(
				new songData("1",66f,0L),  
				new songData("2",70f,0L),  
				new songData("3",75f,-72L),  //sub par
				new songData("4",83f,-5L),
				new songData("5",88f,-8L),  //sub par
				new songData("6",93f,41L),
				new songData("7",97f,-5L),
				new songData("8",104f,-50L),
				new songData("9",110f,-28L),
				new songData("10",113f,-80L),
				new songData("11",117f,-82L), //sub par
				new songData("12",120f,0L),
				new songData("13",125f,-50L),
				new songData("14",129f,-110L),
				new songData("15",132f,-5L),
				new songData("16",135f,-20L),
				new songData("17",138f,5L),
				new songData("18",143f,-20L), //sub par
				new songData("19",148f,-12L),
				new songData("20",153f,0)
				);
		
		classTempos = Arrays.asList(Long.valueOf((long)(60000f/92.1f)), Long.valueOf((long) (60000f/138f)));
		classBiases = Arrays.asList(Long.valueOf(450), Long.valueOf(0));
		
		clickPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BASEPATH + "click.wav")));
		chosenSongPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BASEPATH + "click.wav")));
	}
	
void chooseSongFromIndex(Float i){	
	
	chosenSongPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BCPATH + String.valueOf((int)i.floatValue()) + ".wav")));
	duration = chosenSongPlayer.getDuration(); 
	classIndex = (int)i.floatValue();
}
	
void chooseSongFromTempo(float tempo){
		
	float min = Float.MAX_VALUE;
	int songIndex = -1;
	float dif;
	for(int i = 0; i< songs.size(); i+=1){
		dif = Math.abs(tempo - songs.get(i).tempo);
		if (dif < min){
			min = dif;
			songIndex = i;
		}
	}
chosenSong = songs.get(songIndex);
makeBeatsFromFile(songIndex);
chosenSongPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BASEPATH + chosenSong.fileNumber + ".wav")));
}
	
void playSong(int mode){
		//clickThread.run();
		if(mode == 1) {
		clickThread = new Thread(new Runnable() {
		    public void run() {
		    	    clickPlayer.start();
		    	    
			    	long startTime = System.currentTimeMillis();
			    	long beatTime;
			    	for (int i = 1; i<beats.size(); i=i+1)
			    	{
			    		if (!Thread.interrupted()) {	
			    		beatTime = beats.get(i).longValue();
			    			while(System.currentTimeMillis() - startTime < beatTime)
			    				; //nothing
				    		clickPlayer.start();
			    		}
				    	else break;	
			    	}
			    }
		    });
        clickThread.setPriority(Thread.MAX_PRIORITY);
		}
		else {
			clickThread = new Thread(new Runnable() {
			    public void run() {
			    	long startTime = System.currentTimeMillis();
			    	long tempTime = classTempos.get(classIndex).longValue();
			    	long bias = classBiases.get(classIndex).longValue();

			    	while(System.currentTimeMillis() - startTime < bias){
			    	    ;
			    	}
			    	clickPlayer.start();
			    	    
			    	    int beat = 1;
				    	while (System.currentTimeMillis() - startTime < duration)
				    	{
				    		if (!Thread.interrupted()) {	
				    			while(System.currentTimeMillis() - startTime < tempTime*beat + bias)
				    				; //nothing
					    		clickPlayer.start();
				    		}
					    	else break;	
				    		
				    		beat += 1;
				    	}
				    }
			    });
	        clickThread.setPriority(Thread.MAX_PRIORITY);
			}
		
		 chosenSongPlayer.start();
		 clickThread.start();
}

void stopSong(){

	if(chosenSongPlayer.isPlaying()){
	chosenSongPlayer.stop();
	clickThread.interrupt();
	chosenSongPlayer.reset();
	}
}
	
void makeBeatsFromFile(int si){
	try {
		beats = new ArrayList<Long>();
	    BufferedReader br = new BufferedReader(new FileReader(new File(BASEPATHBEATS + String.valueOf(si+1) + ".txt")));
	    String line;
		    while (!(line = br.readLine()).equals("stop")) {
		        beats.add(Long.valueOf(Float.valueOf(line).longValue() + songs.get(si).bias)); 
	    }
	}
	catch (IOException e) {
		   Log.i(TAG, " makeBeatsFromFile failed miserably with an IOException");
	}
}
	
}
