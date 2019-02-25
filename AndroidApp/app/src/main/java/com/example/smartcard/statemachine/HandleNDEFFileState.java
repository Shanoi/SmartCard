package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;

public class HandleNDEFFileState implements ReadingState {

    private static final String TAG_APDU = "HANDLE NDEF FILE";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////HANDLE FILE////////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        if (commandApdu[INS] == (byte) 0xB0){

            Log.d(TAG_APDU, "Reading Big File");

//            state.readValidContentLength();
            state.setState(new ReadNDEFFileState());
            return state.execute(commandApdu);

        } else if (commandApdu[INS] == (byte) 0xD6){

            Log.d(TAG_APDU, "Updating File");

//            state.readValidContentLength();
            state.setState(new UpdateNDEFFileState());
            return state.execute(commandApdu);

        }

        return INS_UNKNOWN;
    }
}
