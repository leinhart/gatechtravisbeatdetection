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


public class TravisAudioPlayback {

	class songData {
		public final String fileNumber;
		public final float tempo;
		public songData (String fn, float t){
			this.fileNumber = fn;
			this.tempo = t;
		}
	}
	
	private List<songData> songs;
	private List<Long> beatBiases;
	private songData chosenSong;
	private MediaPlayer chosenSongPlayer;
	private MediaPlayer clickPlayer;
	private ArrayList<Long> beats;
	static final String BASEPATH = "mnt/sdcard/Music/TravisDatabase/";
	static final String BASEPATHBEATS = "mnt/sdcard/Music/TravisDatabase/Beats/";
	final Activity owner;
	Context context;
	Thread clickThread;
	
	public TravisAudioPlayback(Activity a){
		owner = a;
		context = a.getApplicationContext();
		beats = new ArrayList<Long>();
		songs = Arrays.asList(
				new songData("1",66f),  
				new songData("2",70f),  
				new songData("3",75f),  
				new songData("4",83f),
				new songData("5",88f),
				new songData("6",93f),
				new songData("7",97f),
				new songData("8",104f),
				new songData("9",110f),
				new songData("10",113f),
				new songData("11",117f),
				new songData("12",120f),
				new songData("13",125f),
				new songData("14",129f),
				new songData("15",132f),
				new songData("16",135f),
				new songData("17",138f),
				new songData("18",143f),
				new songData("19",148f),
				new songData("20",153f)
				);
		beatBiases = Arrays.asList(
				Long.valueOf(20),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0),
				Long.valueOf(0)
				);
		
		clickPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BASEPATH + "click.wav")));
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
songIndex = 0;
chosenSong = songs.get(songIndex);
makeBeatsFromFile(songIndex);
chosenSongPlayer = MediaPlayer.create(context, Uri.fromFile(new File(BASEPATH + chosenSong.fileNumber + ".wav")));
}
	
void playSong(){
	
		chosenSongPlayer.start();
		//clickThread.run();
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
        clickThread.start();
}

void stopSong(){
	
	chosenSongPlayer.stop();
	clickThread.interrupt();
	chosenSongPlayer.reset();
}
	
void makeBeatsFromFile(int si){
	try {
	    BufferedReader br = new BufferedReader(new FileReader(new File(BASEPATHBEATS + String.valueOf(si+1) + ".txt")));
	    String line;
		    while (!(line = br.readLine()).equals("stop")) {
		        beats.add(Long.valueOf(Float.valueOf(line).longValue() + beatBiases.get(si).longValue())); //testing
	    }
	}
	catch (IOException e) {
		   Log.i("makeBeatsFromFile", "Failed miserably with an IOException");
	}
}
	
}
