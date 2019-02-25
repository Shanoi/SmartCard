package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_SELECT;

public class InitialState implements ReadingState {

    private static final String TAG_APDU = "Initial State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////INITIAL STATE//////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        if (commandApdu[P1] == (byte) 0x04 && !state.isApplicationSelected()) {

            Log.d(TAG_APDU, "Set Select App state");

            state.setState(new SelectAppliState());
            return state.execute(commandApdu);

        } else if (commandApdu[P1] == (byte) 0x00 && commandApdu[P2] == (byte) 0x0C && state.isApplicationSelected()) {

            Log.d(TAG_APDU, "Set select file state");

            state.setState(new SelectFileState());
            return state.execute(commandApdu);

        }

        Log.d(TAG_APDU, "P1 / P2 Incorrect for SELECT");
        return P1_2_INCORRECT_SELECT;

    }

}
