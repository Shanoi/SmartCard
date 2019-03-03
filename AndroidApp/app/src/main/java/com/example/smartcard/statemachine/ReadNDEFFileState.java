package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.MLe;
import static com.example.smartcard.HexaValues.ReturnCode.LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OFFSET_LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_READUPDATE;
import static com.example.smartcard.Utility.concateByteArray;

/**
 * This class handles the reading of an NDEF File
 */
public class ReadNDEFFileState implements ReadingState {

    private static final String TAG_APDU = "ReadNDEFFile State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////READ NDEF FILE/////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        int offset = (int) ((commandApdu[P1] & 0xFF) << 8) + (int) (commandApdu[P2] & 0xFF);
        int length = (int) (commandApdu[LC] & 0xFF);

        Log.d(TAG_APDU, "OFFSET + LEN = " + offset + " " + length);
        Log.d(TAG_APDU, "Valid Length = " + state.getFile().getCurrentLength());

        if (offset > state.getFile().getMaxLength()) {
            Log.d(TAG_APDU, "P1 P2 INCORRECT READ / UPDATE INCORRECT");
            return P1_2_INCORRECT_READUPDATE;
        }

        if (offset + length > state.getFile().getCurrentLength()) {
            Log.d(TAG_APDU, "OFFSET LE INCORRECT");
            return OFFSET_LE_INCORRECT;
        }

        int mle = (int) ((MLe[0] & 0xFF) << 8) + (int) (MLe[1] & 0xFF);

        if (length > mle) {
            Log.d(TAG_APDU, "LE INCORRECT");
            return LE_INCORRECT;
        }

        byte[] content = new byte[length];
        byte[] file = state.getFile().getContent();

        for (int i = offset; i < length + offset; i++) {
            content[i - offset] = file[i];
        }

        Log.d(TAG_APDU, "OK CODE ");
        return concateByteArray(content, OK_CODE);

    }
}
