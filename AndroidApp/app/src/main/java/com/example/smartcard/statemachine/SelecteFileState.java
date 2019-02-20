package com.example.smartcard.statemachine;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;

public class SelecteFileState implements ReadingState {
    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        if (commandApdu[P1] == 0x04) {
            state.setState(new InitialState());
            return NO_COMPLIANT_STATE;
        }

        if (commandApdu[5] == 0xE1 && commandApdu[6] == 0x03) {

            state.setCurrentFile(new byte[]{(byte) 0xE1, (byte) 0x03});
//            return concateByteArray(OK_CODE, ccFile());

        }

        return OK_CODE;

    }
}
