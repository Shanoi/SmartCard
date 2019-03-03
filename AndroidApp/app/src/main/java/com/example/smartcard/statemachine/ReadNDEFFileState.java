package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.MLc;
import static com.example.smartcard.HexaValues.CCFile.MLe;
import static com.example.smartcard.HexaValues.ReturnCode.LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OFFSET_LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.concateByteArray;
import static com.example.smartcard.Utility.print;

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

        if (offset + length > state.getFile().getCurrentLength()) {

//            state.setFileSelected(false);
//            state.setState(new InitialState());
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

        /*
        Dans le fichier cCardService, si 0xB0, on sait qu'on est en lecture donc on
        set l'état en lecture si il ne l'est pas déjà, si on change d'état on reset des flags ...
        Même chose pour update
         */


//        if (state.getFile().getCurrentLength() == offset + length) {
//            Log.d(TAG_APDU, "Finished Reading File");
//            state.setState(new InitialState());
//            state.setFileSelected(false);
//        }

        //Log.d(TAG_APDU, "OK CODE " + print(concateByteArray(content, OK_CODE)));
        Log.d(TAG_APDU, "OK CODE ");
        return concateByteArray(content, OK_CODE);

    }
}
