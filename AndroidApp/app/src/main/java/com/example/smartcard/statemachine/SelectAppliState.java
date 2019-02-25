package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.AIDCode.AID1;
import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.ReturnCode.AID_LID_UNKNOWN;
import static com.example.smartcard.HexaValues.ReturnCode.LC_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_SELECT;

public class SelectAppliState implements ReadingState {

    private static final String TAG_APDU = "Select Appli State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////SELECT APPLI///////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        if (commandApdu[P2] != (byte) 0x00) {

            Log.d(TAG_APDU, "P1 / P2 incorrect");

            return P1_2_INCORRECT_SELECT;

        }

        if (commandApdu[LC] > 7 || commandApdu[4] < 5) {

            Log.d(TAG_APDU, "LC Incorrect");

            return LC_INCORRECT;

        }

        for (int i = 0; i < 7; i++) {

            if (commandApdu[5 + i] != AID1[i]) {

                Log.d(TAG_APDU, "LID unknown");

                return AID_LID_UNKNOWN;

            }

        }

        Log.d(TAG_APDU, "Set initial state");

        state.setApplicationSelected(true);
        state.setState(new InitialState());

        Log.d(TAG_APDU, "OK code");

        return OK_CODE;

    }

}
