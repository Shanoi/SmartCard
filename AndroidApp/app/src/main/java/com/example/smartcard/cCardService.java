package com.example.smartcard;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;
import android.util.Log;

public class cCardService extends HostApduService {
    @Override
    public byte[] processCommandApdu(byte[] commandApdu, Bundle extras) {

        return new byte[]{(byte) 0x90, (byte) 0x00};
    }

    @Override
    public void onDeactivated(int reason) {

    }

    @Override
    public void onCreate() {
        super.onCreate();

//        String filename = "myfile";
//        String fileContents = "Hello world!";
//        FileOutputStream outputStream;
//
//        try {
//            outputStream = openFileOutput(filename, Context.MODE_PRIVATE);
//            outputStream.close();
//        } catch (Exception e) {
//            e.printStackTrace();
//        }

    }
}

// Faire une machine d'état

// Créer un CC file en dur et ne plus y toucher
// Le NDEF file doit être créé dans la mémoire internet et contiendra les infos que le lecteur récupère
//   --> gestion au onCreate de classe service
//      --> si existe l'utiliser            Dans la mémoire de l'appli
//      --> si n'existe pas le créer
// 6981
// 6986
// 6A82