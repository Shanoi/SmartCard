package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.CCFile.ccFile;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.concateByteArray;

public class ReadFileState implements ReadingState {

    private static final String TAG_APDU = "Read File State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        if (state.isFileSelected()) {
            Log.d(TAG_APDU, "Set Initiale state");
            state.setState(new InitialState());
            return concateByteArray(ccFile(), OK_CODE);
        }

        Log.d(TAG_APDU, "No compliant state");
        return NO_COMPLIANT_STATE;
    }
}
