package com.example.smartcard;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.example.smartcard.Utility.hexStringToByteArray;
import static com.example.smartcard.Utility.convertToString;

/**
 * This class is used to facilitate the management of the NDEF File by using arrays instead of using the file directly in memory
 */
public class NDEFFile {

    private static final String TAG_APDU = "NDEF FILE";

    /**
     * The current length occupied by the file
     */
    private int currentLength = 0;

    /**
     * The maximum possible length of the NDEF File
     */
    private int maxLength;

    private byte[] content;

    private String filename;

    private File file;

    public NDEFFile(byte[] content, String filename, Context context) {

        this.maxLength = 512;
        this.content = new byte[this.maxLength];

        this.filename = filename;
        this.file = new File(context.getCacheDir(), this.filename);

//      We test if the file exists or not. If the file does not exist, a new one is created with
//      a default content. Otherwise, the file currently in memory is retrieved.
        if (!this.file.exists()) {
            Log.d(TAG_APDU, "FILE NOT EXISTS, CREATING ONE");
            this.currentLength = content.length;
            System.arraycopy(content, 0, this.content, 0, content.length);
            saveFile();
        } else {
            try {
                byte[] c = getContentFromMemory();
                System.arraycopy(c, 0, this.content, 0, c.length);
                this.currentLength = c.length;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

    }

    public byte[] getContent() {
        return content;
    }

    public void setElement(byte element, int index) {
        if (index + 1 > currentLength) {
            currentLength = index + 1;
        }
        content[index] = element;
    }

    public int getCurrentLength() {
        return currentLength;
    }

    public int getMaxLength() {
        return maxLength;
    }

    public void saveFile() {

        try {
            FileOutputStream outputStream = new FileOutputStream(file, false);

            byte[] temp = new byte[currentLength];

            System.arraycopy(content, 0, temp, 0, currentLength);

            outputStream.write(temp);
            outputStream.close();
            Log.d(TAG_APDU, "File SAVED");
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public void saveFile(String str) {

        content = hexStringToByteArray(str);

        saveFile();

    }

    private byte[] getContentFromMemory() throws IOException {


        FileInputStream f = new FileInputStream(file);
        byte[] fileB = new byte[f.available()];

        int index = 0;
        while (f.available() != 0) {

            fileB[index] = (byte) f.read();
            index++;
        }
        Log.d("getContentFromMemory", convertToString(fileB));


        return fileB;

    }

}
