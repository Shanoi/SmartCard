package com.example.smartcard.statemachine;

import android.util.Log;

import java.util.Arrays;

import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.ccFile;
import static com.example.smartcard.HexaValues.ReturnCode.NO_COMPLIANT_STATE;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_READUPDATE;
import static com.example.smartcard.Utility.concateByteArray;

public class ReadFileState implements ReadingState {

    private static final String TAG_APDU = "Read File State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "File selected ?" + state.isFileSelected());
        Log.d("Command received !!!!!", print(commandApdu));

        if (state.isFileSelected()) {

            Log.d(TAG_APDU, "READ FILE STATE" + state.getCurrentFile());

            if (Arrays.equals(state.getCurrentFile(), new byte[]{(byte) 0xE1, (byte) 0x03})) {

                Log.d(TAG_APDU, "Reading CCFile");
                state.setFileSelected(false);
//                    state.setApplicationSelected(false);
                state.setState(new InitialState());
                return concateByteArray(ccFile(), OK_CODE);

            }

            if (Arrays.equals(state.getCurrentFile(), new byte[]{(byte) 0x81, (byte) 0x01})) {

                Log.d(TAG_APDU, "Reading Big File");

                state.readValidContentLength();
                state.setState(new ReadNDEFFileState());

                return state.execute(commandApdu);

            }

//            return P1_2_INCORRECT_READUPDATE;

        }

        Log.d(TAG_APDU, "No compliant state");

        return NO_COMPLIANT_STATE;

    }

    private String print(byte[] bytes) {

        StringBuilder sb = new StringBuilder();
        sb.append("[ ");

        for (byte b : bytes) {

            sb.append(String.format("0x%02X ", b));

        }

        sb.append("]");

        return sb.toString();

    }

}
