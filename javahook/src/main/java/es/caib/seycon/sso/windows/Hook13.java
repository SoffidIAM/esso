package es.caib.seycon.sso.windows;

import java.awt.AWTEvent;
import java.awt.Component;
import java.awt.Frame;
import java.awt.Toolkit;
import java.awt.Window;
import java.awt.event.AWTEventListener;
import java.awt.event.FocusEvent;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.HashMap;

/**
 * Hello world!
 * 
 */
public class Hook13 implements AWTEventListener {
	HashMap savedObjects = new HashMap();
	static Hook13 h;

	public static void main() {
		try {
			AccessController.doPrivileged(new PrivilegedAction() {
				public Object run() {
					try {
						h = new Hook13();
						Toolkit tk = Toolkit.getDefaultToolkit();
						tk.addAWTEventListener(h, AWTEvent.FOCUS_EVENT_MASK);
						Frame frames[] = Frame.getFrames();
						for (int i = 0; frames != null && i < frames.length; i++) {
							if (frames[i].isActive()) {
								Component owner = frames[i].getFocusOwner();
								if (owner != null)
									h.doNotifyFocus(owner);
							}
						}
					} catch (Throwable t) {
						t.printStackTrace(System.out);
					}
					return null;
				}
			});
		} catch (Throwable t) {
			t.printStackTrace(System.out);
		}
	}

	public static void stop() {
		Toolkit tk = Toolkit.getDefaultToolkit();
		tk.removeAWTEventListener(h);
	}

	public Object getObject(long id) {
		return savedObjects.get(new Long(id));
	}

	public void putObject(long id, Object obj) {
		savedObjects.put(new Long(id), obj);
	}

	public void removeObject(long id) {
		savedObjects.remove(new Long(id));
	}

	private native void notifyFocus(Component c);

	class NotifyThread extends Thread {
		Component c;

		public void run() {
			try {
				doNotifyFocus(c);
			} catch (Throwable t) {
				// Ignore
			}
		}

	}

	public void eventDispatched(AWTEvent event) {
		if (event instanceof FocusEvent) {
			FocusEvent fe = (FocusEvent) event;
			if (fe.getID() == FocusEvent.FOCUS_GAINED) {
				NotifyThread nt = new NotifyThread();
				nt.c = fe.getComponent();
				nt.start();
			}
		}
	}

    private void lock()
    {
    	synchronized (this)
    	{
    		locks ++;
    	}
    }
    
    private void unlock ()
    {
    	synchronized (this)
    	{
    		locks --;
    		if (locks == 0)
    			savedObjects.clear(); 
    	}
    }

    int locks = 0;
    private void doNotifyFocus (Component c)
    {
		try {
			lock();
			notifyFocus(c);
		} catch (Throwable t) {
			// Ignore
		} finally {
			unlock();
		}
    }
    
}
