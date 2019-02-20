package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_SELECT;

public class InitialState implements ReadingState {

    private static final String TAG_APDU = "Initial State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        if (commandApdu[2] == 0x04 && !state.isApplicationSelected()) {

            state.setState(new SelectAppliState());
            return state.execute(commandApdu);

        } else if (commandApdu[P1] == 0x00 && commandApdu[P2] == 0x0c && state.isApplicationSelected()) {

            state.setState(new SelecteFileState());
            return state.execute(commandApdu);

        }

        Log.d(TAG_APDU, "P1 / P2 Incorrect for SELECT");
        return P1_2_INCORRECT_SELECT;
    }
}
