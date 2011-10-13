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
import android.widget.TextView;
import android.widget.Toast;

public class TravisBeatDetectionActivity extends Activity implements SharedPreferences.OnSharedPreferenceChangeListener {

	private static final String TAG = "TravisBeatDetectionActivity";
	private PowerManager.WakeLock mWakeLock;
	public TextView tv = null;
	private PdService pdService = null;
	Boolean waitingForGetTempo = false;
	Boolean mediaPlayerStopped = true;
	TravisAudioPlayback audioPlayback;
	//private BluetoothMidiService midiService = null;
	private Toast toast = null;
	
	//GUI listener methods
	public void buttonPressGetTempo(View v) {
		if (!waitingForGetTempo.booleanValue()) {	
			waitingForGetTempo = Boolean.TRUE;
			PdBase.sendFloat("startAudio", 1);	
		}
	}
	
	public void buttonPressStopSong(View v) {
		if ((!waitingForGetTempo.booleanValue()) && (!mediaPlayerStopped.booleanValue())) {
		audioPlayback.stopSong();
		mediaPlayerStopped = Boolean.TRUE;
		}
	}
	
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
		    tv.append(s + ((s.endsWith("\n")) ? "" : "\n"));
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
	  public void receiveList(Object... args) {  
	    for (Object arg: args) {  
	      Log.i("receiveList atom:", arg.toString());  
	    }  
	  }

	  @Override public void receiveSymbol(String symbol) {  
	    Log.i("receiveSymbol", symbol);  
	  }  
	  
	  @Override public void receiveFloat(final float x) { 
	    Log.i("receiveFloat", String.valueOf(x));  
	    if(waitingForGetTempo.booleanValue()){
	    waitingForGetTempo = false;
	    runOnUiThread(new Runnable() {
			@Override
			public void run() {
			tv.setText("tempo: " + String.valueOf(x));
			audioPlayback.chooseSongFromTempo(x);
			audioPlayback.playSong();
			mediaPlayerStopped = Boolean.FALSE;
			//PdBase.sendFloat("startAudio", 0);
			}
		});
	   }
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
		audioPlayback = new TravisAudioPlayback(this);
	};

	@Override
	protected void onDestroy() {
		super.onDestroy();
		mWakeLock.release();
		cleanup();
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		startAudio();
	}

	private void initGui() {
		setContentView(R.layout.main);
		tv = (TextView) findViewById(R.id.textview);
		tv.setMovementMethod(new ScrollingMovementMethod());
	}

	private void initPd() {
		Resources res = getResources();
		File patchFile = null;
		try {
			PdBase.setReceiver(myDispatcher);
			//make tempo the send I listen to
			myDispatcher.addListener("tempo", myListener);
			myDispatcher.addListener("testbang", myListener);
			PdBase.subscribe("android");
			File libDir = getFilesDir();
			try {
				IoUtils.extractZipResource(res.openRawResource(R.raw.externals), libDir, true);}
				catch (IOException e) {
					Log.e("Scene Player", e.toString());
				}
			PdBase.addToSearchPath(libDir.getAbsolutePath());
			InputStream in = res.openRawResource(R.raw.androidbeatdetection);
			patchFile = IoUtils.extractResource(in, "androidbeatdetection.pd", getCacheDir());
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
		PdBase.release();
		try {
			unbindService(pdConnection);
		} catch (IllegalArgumentException e) {
			// already unbound
			pdService = null;
		}
		audioPlayback.stopSong();
	}

	@Override
	public void finish() {
		cleanup();
		super.finish();
	}

}