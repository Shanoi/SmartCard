package com.example.smartcard.statemachine;

import android.content.Context;
import android.util.Log;

import static com.example.smartcard.HexaValues.CCFile.NDEFFile;

public class State {

    private ReadingState state;

    private boolean applicationSelected;

    private byte[] currentFile;
    private boolean fileSelected;

    private byte[] fileContent;
    private int validContentLength;

    private static final String TAG_APDU = "STATE";

    private Context context;

    public State(ReadingState state, Context context) {
        this.state = state;
        applicationSelected = false;
        this.validContentLength = 0;
        this.context = context;
//        int a = (int) (fileContent[0] << 8);
//        int b = (int) (fileContent[1]);
//
//        validContentLength = a + b;
    }

    public ReadingState getState() {
        return state;
    }

    public void setState(ReadingState state) {
        this.state = state;
    }

    public boolean isApplicationSelected() {
        return applicationSelected;
    }

    public void setApplicationSelected(boolean applicationSelected) {
        this.applicationSelected = applicationSelected;
    }

    public byte[] getCurrentFile() {
        return currentFile;
    }

    public void setCurrentFile(byte[] currentFile) {
        this.currentFile = currentFile;
    }

    public void setFileSelected(boolean b) {
        this.fileSelected = b;
    }

    public boolean isFileSelected() {
        return fileSelected;
    }

    public byte[] getFileContent() {
        return fileContent;
    }

    public void setFileContent(byte[] fileContent) {
        this.fileContent = fileContent;
    }

    public void setFileContent(byte[] content, int offset) {

        System.arraycopy(content, 0, fileContent, offset + 0, content.length);

    }

    public int getValidContentLength() {
        return validContentLength;
    }

    public void readValidContentLength() {

        int a = (int) (fileContent[0] << 8);
        int b = (int) (fileContent[1]);

        Log.d(TAG_APDU, "" + fileContent[0]);
        Log.d(TAG_APDU, "" + fileContent[1]);
        Log.d(TAG_APDU, "" + a);
        Log.d(TAG_APDU, "" + b);

        validContentLength = a + b;

    }

    public Context getContext() {
        return context;
    }

    public byte[] execute(byte[] commandApdu) {
        return state.apply(this, commandApdu);
    }

}
