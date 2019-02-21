package com.example.smartcard.statemachine;

public class State {

    private ReadingState state;

    private boolean applicationSelected;

    private byte[] currentFile;
    private boolean fileSelected;

    public State(ReadingState state) {
        this.state = state;
        applicationSelected = false;
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

    public byte[] execute(byte[] commandApdu) {
        return state.apply(this, commandApdu);
    }

}
