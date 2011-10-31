package bd.travisbeatdetection;


import java.io.File;
import java.io.IOException;
import java.io.InputStream;

import org.puredata.android.service.PdPreferences;
import org.puredata.android.service.PdService;
import org.puredata.core.PdBase;
import org.puredata.core.utils.IoUtils;
import org.puredata.core.utils.PdDispatcher;
import org.puredata.core.utils.PdListener;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.IBinder;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

public class TravisBeatDetectionActivity extends Activity implements SharedPreferences.OnSharedPreferenceChangeListener{

	private static final String TAG = "TravisBeatClassificationActivity";
	private PowerManager.WakeLock mWakeLock;
	public TextView progBarText = null;
	public ProgressBar progressBar = null;
	public Button buttonUp = null;
	public Button buttonDown = null;
	private PdService pdService = null;
	private float tempo;
	private int mode;
	Boolean waitingForGetTempo = false;
	Boolean mediaPlayerStopped = true;
	TravisBCAudioPlayback audioPlayback;
	//private BluetoothMidiService midiService = null;
	private Toast toast = null;
	private long songStartDelays[] = {130, 895};
	
	private void toast(final String msg) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (toast == null) {
					toast = Toast.makeText(getApplicationContext(), "", Toast.LENGTH_SHORT);
				}
				toast.setText(TAG + ": " + msg);
				toast.show();
			}
		});
	}

	private void post(final String s) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
		    //tv.append(s + ((s.endsWith("\n")) ? "" : "\n"));
			}
		});
	}
	
	private final PdDispatcher myDispatcher = new PdDispatcher() {  
		  @Override  
		  public void print(String s) {  
		    Log.i("Pd print", s);  
		  }  
		};

		private final PdListener myListener = new PdListener() {  
	  @Override  
	  public void receiveMessage(String symbol, Object... args) {  
	    Log.i("receiveMessage symbol:", symbol);  
	    for (Object arg: args) {  
	      Log.i("receiveMessage atom:", arg.toString());  
	    }  
	  }
 
	  @Override  
	  public void receiveList(final Object... args) {   
	      Log.i("receiveList atoms:", args[0].toString());  
	      if(Float.valueOf(args[0].toString()) == -2.0) {
	    	  
	    	  runOnUiThread(new Runnable() {
		  	    	
		  			@Override
		  			public void run() {
		  			//start listening for inputs
		  	    	  if (!waitingForGetTempo.booleanValue()) {	
		  	  			waitingForGetTempo = Boolean.TRUE;
		  	  		  }
		  	    	  //make an animation to let user know it is working
		  	    	  progressBar.setVisibility(0);
		  	    	  progBarText.setText("Detecting Beat");
		  	    	  progBarText.setVisibility(0);
		  			}
		  		});
	    	  
	      }
	      else if(Float.valueOf(args[0].toString()) == -1.0) {
	    	  if ((!waitingForGetTempo.booleanValue()) && (!mediaPlayerStopped.booleanValue())) {
	    			audioPlayback.stopSong();
	    			mediaPlayerStopped = Boolean.TRUE;
	    			runOnUiThread(new Runnable() {
			  	    	
			  			@Override
			  			public void run() {
			  				progBarText.setText("Clap To Start");
			  			}
	    			});	
	    		}
	    	  
	    	  //play song
//	    	  if(waitingForGetTempo.booleanValue()){
//	    	  runOnUiThread(new Runnable() {
//		  			@Override
//		  			public void run() {
//					try {
//						Thread.sleep((long)(60/tempo * 1000 - 300));  //I deserve to be maimed for this.
//					} catch (InterruptedException e) {
//						// TODO Auto-generated catch block
//						e.printStackTrace();
//					}
//		  			audioPlayback.playSong();
//		  			tv.append("\nnow playing");
//		  			mediaPlayerStopped = Boolean.FALSE;
//		  			}
//		  		});
//	    	  waitingForGetTempo = false;
//	    	  }
	    	  
	      }
	      else {
	    	  if(waitingForGetTempo.booleanValue()){
	  	    runOnUiThread(new Runnable() {
	  	    	
	  			@Override
	  			public void run() {
	  				progressBar.setVisibility(4);
		  	    	  progBarText.setText("Playing Song\nClap To Stop");
		  	    	  //progBarText.setVisibility(4);	
	  			tempo = Float.valueOf(args[0].toString());
	  		    // 0 = classification, 1 = detection
	  			//tv.setText("song: " + args[0].toString());
	  			try {
	  				if (tempo < 2) {
	  					audioPlayback.chooseSongFromIndex(Float.valueOf(args[0].toString()));	
	  				Thread.sleep((long)(songStartDelays[(int)tempo])); 
	  				mode = 0;
	  				}
	  				else {
	  					audioPlayback.chooseSongFromTempo(tempo);
	  				mode = 1;
	  				}
	  			}
				 catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
	  			audioPlayback.playSong(mode);
	  			//tv.append("\nnow playing");
	  			mediaPlayerStopped = Boolean.FALSE;
	  			//PdBase.sendFloat("startAudio", 0);
	  			}
	  		});
	  	    
	  	  waitingForGetTempo = false;
	     }
	    }
	  }

	  @Override public void receiveSymbol(String symbol) {  
	    Log.i("receiveSymbol", symbol);  
	  }  
	  
	  @Override public void receiveFloat(final float x) { 
	    Log.i("receiveFloat", String.valueOf(x));  
	  }  
	   
	  @Override public void receiveBang() {  
	    Log.i("receiveBang", "bang!");  
	  }  
	};

	private final ServiceConnection pdConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			pdService = ((PdService.PdBinder)service).getService();
			initPd();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			// this method will never be called
		}
	};

	@Override
	protected void onCreate(android.os.Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
		mWakeLock.acquire();
		PdPreferences.initPreferences(getApplicationContext());
		PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).registerOnSharedPreferenceChangeListener(this);
		initGui();
		bindService(new Intent(this, PdService.class), pdConnection, BIND_AUTO_CREATE);	
		audioPlayback = new TravisBCAudioPlayback(this);
	};

	@Override
	protected void onDestroy() {
		super.onDestroy();
		android.os.Process.killProcess(android.os.Process.myPid()); //perhaps to change later - this app won't run in the background
		mWakeLock.release();
		cleanup();
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		startAudio();
	}
	

	private void initGui() {
		setContentView(R.layout.main);
		setProgressBarVisibility(false);
		progBarText = (TextView) findViewById(R.id.textView1);
		buttonUp = (Button) findViewById(R.id.button1);
		buttonUp.setOnClickListener(new OnClickListener() {
		    @Override
		    public void onClick(View v) {
		    	PdBase.sendFloat("threshUp", 1);
		    }
		  });
		buttonDown = (Button) findViewById(R.id.button2);
		buttonDown.setOnClickListener(new OnClickListener() {
		    @Override
		    public void onClick(View v) {
		    	PdBase.sendFloat("threshDown", 1);
		    }
		  });
		//tv.setMovementMethod(new ScrollingMovementMethod());
		progressBar = (ProgressBar) findViewById(R.id.progressBar1);
		progressBar.setVisibility(4);
		progBarText.setText("Clap To Start");
	}

	private void initPd() {
		Resources res = getResources();
		File patchFile = null;
		try {
			PdBase.setReceiver(myDispatcher);
			//make tempo the send I listen to
			myDispatcher.addListener("tempo", myListener);
			//myDispatcher.addListener("testbang", myListener);
			PdBase.subscribe("android");
			File libDir = getFilesDir();
			try {
				IoUtils.extractZipResource(res.openRawResource(R.raw.externals), libDir, true);}
				catch (IOException e) {
					Log.e("Unzip externals", e.toString());
				}
			PdBase.addToSearchPath(libDir.getAbsolutePath());
			InputStream in = res.openRawResource(R.raw.androidbeatclassification);
			patchFile = IoUtils.extractResource(in, "androidbeatclassification.pd", getCacheDir());
			PdBase.openPatch(patchFile);
			startAudio();
			//PdBase.sendFloat("startAudio", 0);
		} catch (IOException e) {
			Log.e(TAG, e.toString());
			finish();
		} finally {
			if (patchFile != null) patchFile.delete();
		}
	}

	private void startAudio() {
		String name = getResources().getString(R.string.app_name);
		try {
			pdService.initAudio(-1, -1, -1, -1);   // negative values will be replaced with defaults/preferences
			pdService.startAudio(new Intent(this, TravisBeatDetectionActivity.class), R.drawable.icon, name, "Return to " + name + ".");
		} catch (IOException e) {
			toast(e.toString());
		}
	}

	private void cleanup() {
		audioPlayback.stopSong();
		PdBase.release();
		try {
			//pdService.stopSelf();
			unbindService(pdConnection);
		} catch (IllegalArgumentException e) {
			// already unbound
			pdService = null;
		}
	}

	
	@Override
	public void finish() {
		//cleanup();
		super.finish();
		onDestroy();
	}
}