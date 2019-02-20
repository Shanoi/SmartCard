package com.example.smartcard.statemachine;

public class ReadFile implements ReadingState {
    @Override
    public byte[] apply(State state, byte[] commandApdu) {
        return new byte[0];
    }
}
