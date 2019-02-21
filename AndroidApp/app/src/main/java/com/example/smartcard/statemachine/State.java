package com.example.smartcard.statemachine;

import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Map;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.NDEFFile;

public class State {

    private ReadingState state;

    private boolean applicationSelected;

    private byte[] currentFile;
    private boolean fileSelected;

    private byte[] file;
    private int validContentLength;

    private static final String TAG_APDU = "STATE";

    public State(ReadingState state) {
        this.state = state;
        applicationSelected = false;
        this.validContentLength = 0;
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

    public byte[] getFile() {
        return file;
    }

    public void setFile(byte[] file) {
        this.file = file;
    }

    public int getValidContentLength() {
        return validContentLength;
    }

    public void readValidContentLength() {

        int a = (int) (NDEFFile[4] << 8);
        int b = (int) (NDEFFile[5]);

        Log.d(TAG_APDU, "" + file[0]);
        Log.d(TAG_APDU, "" + file[1]);
        Log.d(TAG_APDU, "" + a);
        Log.d(TAG_APDU, "" + b);

        validContentLength = a + b;

    }

    public byte[] execute(byte[] commandApdu) {
        return state.apply(this, commandApdu);
    }

}
