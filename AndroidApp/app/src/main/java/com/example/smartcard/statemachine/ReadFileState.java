package com.example.smartcard.statemachine;

import android.util.Log;

import java.util.Arrays;

import static com.example.smartcard.HexaValues.CCFile.ccFile;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.concateByteArray;

/**
 * This class represents the state when the CCFile is read.
 */
public class ReadFileState implements ReadingState {

    private static final String TAG_APDU = "Read File State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////READ FILE//////////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        if (state.isFileSelected()) {

            Log.d(TAG_APDU, "READ FILE STATE" + state.getCurrentFile());

            if (Arrays.equals(state.getCurrentFile(), new byte[]{(byte) 0xE1, (byte) 0x03})) {

                Log.d(TAG_APDU, "Reading CCFile");
                state.setFileSelected(false);
                state.setState(new InitialState());
                return concateByteArray(ccFile(), OK_CODE);

            }

        }

        Log.d(TAG_APDU, "No compliant state");

        return NO_COMPLIANT_STATE;

    }

}
