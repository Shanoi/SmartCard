package com.example.smartcard.statemachine;

import android.content.Context;

import com.example.smartcard.NDEFFile;

/**
 * This class represents the current state of the emulated card
 */
public class State {

    /**
     * The current state
     */
    private ReadingState state;

    /**
     * To know if the application has been selected
     */
    private boolean applicationSelected;

    /**
     * The identifier of the current file
     */
    private byte[] currentFile;

    /**
     * To know if a file is selected
     */
    private boolean fileSelected;

    /**
     * The NDEF File that the application has selected
     */
    private NDEFFile file;

    private static final String TAG_APDU = "STATE";

    private Context context;

    public State(ReadingState state, Context context, byte[] content, String filename) {
        this.state = state;
        applicationSelected = false;
        this.file = new NDEFFile(content, filename, context);
        this.context = context;
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

    public NDEFFile getFile() {
        return file;
    }

    public void setFile(NDEFFile file) {
        this.file = file;
    }

    public Context getContext() {
        return context;
    }

    public byte[] execute(byte[] commandApdu) {
        return state.apply(this, commandApdu);
    }

}
