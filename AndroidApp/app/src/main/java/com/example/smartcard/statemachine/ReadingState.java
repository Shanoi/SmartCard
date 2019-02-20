package com.example.smartcard.statemachine;

@FunctionalInterface
public interface ReadingState  {
    byte[] apply(State state, byte[] commandApdu);
}
