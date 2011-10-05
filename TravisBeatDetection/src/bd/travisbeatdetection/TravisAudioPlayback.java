package bd.travisbeatdetection;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


public class TravisAudioPlayback {

	class songData {
		public final String fileNumber;
		public final int tempo;
		public songData (String fn, int t){
			this.fileNumber = fn;
			this.tempo = t;
		}
	}
	
	private List<songData> songs;
	
	void init(){
		songs = Arrays.asList(
				new songData("1",66),  
				new songData("2",70),  
				new songData("3",75),  
				new songData("4",83),
				new songData("5",88),
				new songData("6",93),
				new songData("7",97),
				new songData("8",104),
				new songData("9",110),
				new songData("10",113),
				new songData("11",117),
				new songData("12",120),
				new songData("13",125),
				new songData("14",129),
				new songData("15",132),
				new songData("16",135),
				new songData("17",138),
				new songData("18",143),
				new songData("19",148),
				new songData("20",153)
				);	
	}
	
	void chooseSongFromTempo(){
		
		
	}
	
	void playSong(){
		
		
	}
	
}
