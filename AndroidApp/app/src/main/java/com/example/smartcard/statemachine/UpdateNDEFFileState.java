package com.example.smartcard.statemachine;

import android.util.Log;

import java.util.ArrayList;

import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.MLc;
import static com.example.smartcard.HexaValues.ReturnCode.LC_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OFFSET_LC_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_READUPDATE;

/**
 * This class handle the update of NDEF File
 */
public class UpdateNDEFFileState implements ReadingState {

    private static final String TAG_APDU = "UPDATE NDEF FILE";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        ArrayList<Integer> al = new ArrayList<>();

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////UPDATE FILE////////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        int offset = (int) ((commandApdu[P1] & 0xFF) << 8) + (int) (commandApdu[P2] & 0xFF);
        int incomingLC = commandApdu[LC] & 0xFF;

        int mlc = (int) ((MLc[0] & 0xFF) << 8) + (int) (MLc[1] & 0xFF);

        if (incomingLC > mlc) {
            Log.d(TAG_APDU, "LC INCORRECT");
            return LC_INCORRECT;
        }

        if (offset > state.getFile().getMaxLength()){
            Log.d(TAG_APDU, "P1 P2 INCORRECT READ / UPDATE INCORRECT");
            return P1_2_INCORRECT_READUPDATE;
        }

        if (offset + incomingLC > state.getFile().getMaxLength()) {
            Log.d(TAG_APDU, "OFFSET LC INCORRECT");
            return OFFSET_LC_INCORRECT;
        }

//      We create an array to copy the data without the header of the APDU command
        byte[] data = new byte[commandApdu.length - 5];

        System.arraycopy(commandApdu, 5, data, 0, incomingLC);

        Log.d(TAG_APDU, "OFFSET = " + offset);
        Log.d(TAG_APDU, "DATA LENGTH = " + data.length);

        for (int i = 0; i < data.length; i++) {

            state.getFile().setElement(data[i], offset + i);

        }

        Log.d(TAG_APDU, "OK CODE DATA WRITTEN");
        return OK_CODE;

    }
}
