package bd.travisbeatsmaker;

import java.awt.BorderLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JFrame;
import javax.swing.JTextArea;
import javax.swing.JTextField;

public class BeatsMaker extends JFrame implements KeyListener, MouseListener{

	/**
	 * 
	 */
	JTextArea textArea;
	Boolean isStart = true;
	long startTime;
	long beat;
	//long lastBeat;

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
			init();
			while(true){
				;
			}
	}
	
	static void init()
	{
		BeatsMaker frame = new BeatsMaker("BeatsMaker");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        //frame.addComponentsToPane();
        frame.addKLToFrame();
        frame.pack();
        frame.setVisible(true);
		//typingArea = new JTextField(20);
        //typingArea.
		
	}
	
	public BeatsMaker(String name) {
        super(name);
    }

	void addKLToFrame(){
		textArea = new JTextArea(50,20);
        textArea.addKeyListener(this);
        textArea.addMouseListener(this);
        getContentPane().add(textArea, BorderLayout.PAGE_START);
	}
	
	@Override
	public void keyPressed(KeyEvent arg0) {
		
//		if(isStart.booleanValue() == true) {
//			System.out.print("0\n");
//			isStart = false;
//			startTime = System.currentTimeMillis();
//		}
//		else {
//			beat = System.currentTimeMillis();
//			System.out.print(String.valueOf((beat - startTime)) + "\n");	
//		}
	}

	@Override
	public void keyReleased(KeyEvent arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void keyTyped(KeyEvent arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void mouseClicked(MouseEvent arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void mouseEntered(MouseEvent arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void mouseExited(MouseEvent arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void mousePressed(MouseEvent arg0) {
		// TODO Auto-generated method stub
		if(isStart.booleanValue() == true) {
			System.out.print("0\n");
			//lastBeat = 0L;
			isStart = false;
			startTime = System.currentTimeMillis();
		}
		else {
			beat = System.currentTimeMillis();
			beat -= startTime;
			System.out.print(String.valueOf((beat)) + "\n");
			//lastBeat = beat;
		}
	}

	@Override
	public void mouseReleased(MouseEvent arg0) {
		// TODO Auto-generated method stub
		
	}
	
}
