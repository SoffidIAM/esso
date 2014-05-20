package es.caib.seycon.sso.windows;

import java.awt.Component;
import java.awt.KeyboardFocusManager;
import java.awt.Window;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.net.URLClassLoader;
import java.util.HashMap;

import javax.imageio.spi.RegisterableService;

/**
 * Hello world!
 *
 */
public class Hook implements PropertyChangeListener 
{
	static KeyboardFocusManager kfm;
	HashMap savedObjects = new HashMap();
	
    public static void main( )
    {
    	Hook h = new Hook();
    	kfm = KeyboardFocusManager.getCurrentKeyboardFocusManager();
    	kfm.addPropertyChangeListener("focusOwner", h);
    	kfm.addPropertyChangeListener("managingFocus", h);
    	h.notifyFocus (kfm.getFocusedWindow(), kfm.getFocusOwner());
    }

    public static void stop( )
    {
    	kfm = KeyboardFocusManager.getCurrentKeyboardFocusManager();
    	PropertyChangeListener[] cl = kfm.getPropertyChangeListeners("focusOwner");
    	for (int i = 0; i < cl.length; i++)
    	{
    		if (cl[i] != null &&
    			cl[i].getClass().getName().equals(Hook.class.getName()))
    		{
    			kfm.removePropertyChangeListener("focusOwner", cl[i]);
    		}
    	}
    }
    
    public Object getObject (long id) {
    	return savedObjects.get(new Long(id));
    }

    public void putObject (long id, Object obj) {
    	savedObjects.put(new Long(id), obj);
    }

    public void removeObject (long id) {
    	savedObjects.remove(new Long(id));
    }

    public void propertyChange(PropertyChangeEvent evt) {
    	if ("focusOwner".equals(evt.getPropertyName())) {
			NotifyThread nt = new NotifyThread();
			nt.w = kfm.getFocusedWindow();
			nt.c = kfm.getFocusOwner();
			nt.start();
    	} else if ("managingFocus".equals(evt.getPropertyName()) &&
    			evt.getNewValue().equals(Boolean.FALSE)) {
        	kfm = KeyboardFocusManager.getCurrentKeyboardFocusManager();
        	kfm.addPropertyChangeListener("focusOwner", this);
        	kfm.addPropertyChangeListener("managingFocus", this);
        	this.notifyFocus (kfm.getFocusedWindow(), kfm.getFocusOwner());
    	}
	}

	private native void notifyFocus(Window w, Component c) ;

	class NotifyThread extends Thread {
		Window w;
		Component c;
		public void run() {
			try {
				notifyFocus(w, c);
			} catch (Throwable t) {
				// Ignore
			}
		}
		
	}
}


