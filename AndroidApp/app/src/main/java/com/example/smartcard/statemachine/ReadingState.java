package com.example.smartcard.statemachine;

/**
 * This is the interface to implement the State Machine
 */
@FunctionalInterface
public interface ReadingState {

    byte[] apply(State state, byte[] commandApdu);

}
