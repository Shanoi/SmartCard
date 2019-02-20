package com.example.smartcard.statemachine;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;

public class SelecteFileState implements ReadingState {
    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        if (commandApdu[P1] == 0x04) {
            return NO_COMPLIANT_STATE;
        }

        return OK_CODE;

    }
}
