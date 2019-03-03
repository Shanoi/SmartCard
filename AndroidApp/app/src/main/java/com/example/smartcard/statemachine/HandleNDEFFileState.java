package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.INS;
import static com.example.smartcard.HexaValues.ReturnCode.INS_UNKNOWN;

/**
 * Class to handle the NDEF File. This class will either redirect to the reading or the updating of the NDEF File
 */
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

            state.setState(new ReadNDEFFileState());
            return state.execute(commandApdu);

        } else if (commandApdu[INS] == (byte) 0xD6){

            Log.d(TAG_APDU, "Updating File");

            state.setState(new UpdateNDEFFileState());
            return state.execute(commandApdu);

        }

        return INS_UNKNOWN;
    }
}
