package de.citrad.companion;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDeviceConnection;
import android.util.Log;

import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;
import java.util.Locale;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

// USB permission stuff
import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

public class Serial
{
    private enum UsbPermission { Unknown, Requested, Granted, Denied }

    private static final String TAG = "Serial";
    private static final String INTENT_ACTION_GRANT_USB = "PLEASE.GIMME.USB";

    private UsbPermission usbPermission = UsbPermission.Unknown;
    private List<UsbSerialDriver> availableDrivers;
    private boolean discovered = false;
    private UsbSerialPort port;
    private byte[] buffer;
    private boolean hadError = false;

    public int getBufferSize() {
        return buffer.length;
    }

    public int discoverUsb(Context context)
    {
        try
        {
            Log.i(TAG, "CITRAD COMPANION searching for devices");

            UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
            availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
            discovered = true;
            if (availableDrivers.isEmpty()) {
                return 0;
            }

            int count = 0;
            for(int i = 0; i < availableDrivers.size(); i++) {
                count += availableDrivers.get(i).getPorts().size();
            }
            Log.d(TAG, "found " + count + " devices");
            return count;
        }
        catch (Exception e) {
            e.printStackTrace();
            return 0;
        }
    }

    public String getDeviceName(int idx)
    {
        if(idx < 0) return "";

        int count = 0;
        for(int i = 0; i < availableDrivers.size(); i++) {
            for(int p = 0; p < availableDrivers.get(i).getPorts().size(); p++)
            {
                if(idx == count)
                {
                    UsbSerialDriver driver = availableDrivers.get(i);
                    UsbDevice d = driver.getDevice();

                    return driver.getClass().getSimpleName().replace("SerialDriver","") +
                            " " + d.getManufacturerName() +
                            "/" + d.getProductName()
                            + " Port " + p;
                }

                count++;
            }
        }

        return "";
    }

    public boolean connectDevice(Context context, Activity activity, int idx)
    {
        if(discovered == false)
            discoverUsb(context);

        if(idx < 0)
        {
            Log.w(TAG, "invalid driver index");
            return false;
        }

        try
        {
            UsbSerialDriver driver = availableDrivers.get(0);
            int portIdx = -1;

            int count = 0;
            for(int i = 0; i < availableDrivers.size(); i++) {
                for(int p = 0; p < availableDrivers.get(i).getPorts().size(); p++)
                {
                    if(idx == count)
                    {
                        driver = availableDrivers.get(i);
                        portIdx = p;
                        break;
                    }
                    count++;
                }
                if(portIdx != -1)
                    break;
            }
            if(portIdx == -1)
            {
                Log.w(TAG, "invalid driver index");
                return false;
            }

            // Open a connection to the first available driver.
            UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
            UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
            if(connection == null && usbPermission == UsbPermission.Unknown && !manager.hasPermission(driver.getDevice())) {
                usbPermission = UsbPermission.Requested;
                Intent intent = new Intent(INTENT_ACTION_GRANT_USB);
                intent.setPackage("de.citrad.companion");
                PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(activity, 0, intent, 0);
                manager.requestPermission(driver.getDevice(), usbPermissionIntent);

                Log.i(TAG, "asking for permission");
                return false;
            }
            if(connection == null) {
                if(!manager.hasPermission(driver.getDevice()))
                {
                    Log.w(TAG, "connection failed: permission denied");
                    return false;
                }

                Log.w(TAG, "connection failed: open failed");
                return false;
            }

            port = driver.getPorts().get(portIdx);
            port.open(connection);
            port.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            port.setDTR(true);
            Log.d(TAG, "connected to UsbSerial");

            buffer = new byte[port.getReadEndpoint().getMaxPacketSize()];

            return true;
        }
        catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public byte[] readBatch()
    {
        if(port == null || port.isOpen() == false)
            return new byte[0];

        try
        {
            int len = port.read(buffer, 200);
            if(len == 0)
                return new byte[0];

            return Arrays.copyOf(buffer, len);
        }
        catch (Exception e) {
            e.printStackTrace();
            hadError = true;
        }

        return new byte[0];
    }

    public boolean hasError() { return hadError; }

    public void close()
    {
        try
        {
            if(port != null && port.isOpen())
                port.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
}
