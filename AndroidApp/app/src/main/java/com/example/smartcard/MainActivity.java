package com.example.smartcard;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import static com.example.smartcard.Utility.print;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    public void onClick(View view) {

        Log.d("MAIN", "Click");

        File file = new File(getBaseContext().getCacheDir(), "NDEFFile");

        try {

            FileInputStream f = new FileInputStream(file);
            byte[] fileB = new byte[f.available()];

            int index = 0;
            while (f.available() != 0){

                fileB[index] = (byte) f.read();
                index++;
            }
            Log.d("FILE CONTENT", print(fileB));

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }
}
