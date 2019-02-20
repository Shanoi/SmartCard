package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;

public class SelecteFileState implements ReadingState {

    private static final String TAG_APDU = "Select File State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        if (commandApdu[P1] == (byte) 0x04) {
            Log.d(TAG_APDU, "No compliante state");
            state.setState(new InitialState());
            return NO_COMPLIANT_STATE;
        }

        if (commandApdu[5] == (byte) 0xE1 && commandApdu[6] == (byte) 0x03) {

            Log.d(TAG_APDU, "Set read file state");
            state.setFileSelected(true);
            state.setCurrentFile(new byte[]{(byte) 0xE1, (byte) 0x03});
            state.setState(new ReadFileState());
//            return concateByteArray(OK_CODE, ccFile());

        }
        Log.d(TAG_APDU, "OK CODE");
        return OK_CODE;

    }
}
