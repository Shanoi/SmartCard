package com.example.smartcard.statemachine;

public class State {

    private ReadingState state;

    public State(ReadingState state) {
        this.state = state;
    }

    public ReadingState getState() {
        return state;
    }

    public void setState(ReadingState state) {
        this.state = state;
    }

    public byte[] executeState(byte[] commandApdu) {

        return state.apply(this, commandApdu);

    }

}

