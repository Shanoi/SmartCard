package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.ReturnCode.AID_LID_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;

/**
 * This class will select the CCFile or tghe NDEF File.
 */
public class SelectFileState implements ReadingState {

    private static final String TAG_APDU = "Select File State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////SELECT FILE////////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        if (commandApdu[P1] == (byte) 0x04) {
            Log.d(TAG_APDU, "No compliante state");
            state.setState(new InitialState());
            return NO_COMPLIANT_STATE;
        }

//      Select the CCFile
        if (commandApdu[5] == (byte) 0xE1 && commandApdu[6] == (byte) 0x03) {

            Log.d(TAG_APDU, "Set read file state");

            state.setFileSelected(true);
            state.setCurrentFile(new byte[]{(byte) 0xE1, (byte) 0x03});
            state.setState(new ReadFileState());

            Log.d(TAG_APDU, "OK CODE");
            return OK_CODE;

//       Select the NDEF File
        } else if (commandApdu[5] == (byte) 0x81 && commandApdu[6] == (byte) 0x01) {

            state.setFileSelected(true);
            state.setCurrentFile(new byte[]{(byte) 0x81, (byte) 0x01});
            state.setState(new HandleNDEFFileState());

            Log.d(TAG_APDU, "OK CODE");
            return OK_CODE;

        } else {

            return AID_LID_UNKNOWN;
        }

    }

}
