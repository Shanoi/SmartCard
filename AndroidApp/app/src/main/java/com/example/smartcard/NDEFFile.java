package com.example.smartcard;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.example.smartcard.Utility.hexStringToByteArray;
import static com.example.smartcard.Utility.print;

public class NDEFFile {

    private static final String TAG_APDU = "NDEF FILE";

    private int currentLength = 0;
    private int maxLength;

    private byte[] content;

    private String filename;

    private File file;

    public NDEFFile(byte[] content, String filename, Context context) {

//        this.content = content;
        this.maxLength = 512;
        this.content = new byte[this.maxLength];

        this.filename = filename;
        this.file = new File(context.getCacheDir(), "NDEFFile");

        if (!this.file.exists()) {
            Log.d(TAG_APDU, "FILE NOt EXISTS");
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
//            003191010A55016170706C652E636F6D510114540266724C612062656C6C6520686973746f697265510008504F4C5954454348

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

    public String getFileContent() {

        try {

            FileInputStream f = new FileInputStream(file);
            byte[] fileB = new byte[f.available()];

            int index = 0;
            while (f.available() != 0) {

                fileB[index] = (byte) f.read();
                index++;
            }
            Log.d(TAG_APDU, "FILE CONTENT " + print(fileB));

            return print(fileB);

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return "Fail Read";
    }

    private byte[] getContentFromMemory() throws IOException {


        FileInputStream f = new FileInputStream(file);
        byte[] fileB = new byte[f.available()];

        int index = 0;
        while (f.available() != 0) {

            fileB[index] = (byte) f.read();
            index++;
        }
        Log.d("getContentFromMemory", print(fileB));


        return fileB;

    }

}