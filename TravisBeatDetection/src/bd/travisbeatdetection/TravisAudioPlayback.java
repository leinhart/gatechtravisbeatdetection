package bd.travisbeatdetection;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.io.File;
import java.lang.Math;

import android.app.Activity;
import android.media.MediaPlayer;
import android.net.Uri;


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
	private songData chosenSong;
	private MediaPlayer chosenSongPlayer;
	static final String BASEPATH = "mnt/sdcard/Music/TravisDatabase/";
	final Activity owner;
	
	public TravisAudioPlayback(Activity a){
		owner = a;
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
chosenSongPlayer = MediaPlayer.create(owner.getApplicationContext(), Uri.fromFile(new File(BASEPATH + chosenSong.fileNumber + ".wav")));
}
	
	void playSong(){
		chosenSongPlayer.start();
	}
	
}
